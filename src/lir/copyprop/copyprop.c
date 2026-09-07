#include <lir/copyprop.h>

/* Add variables referenced by a subject to the USE set.
Params:
    - `use` - USE set to update.
    - `arg` - LIR subject to inspect.

Returns 1 if subject was handled, otherwise 0 */
static int _mark_used_var(set_t* use, lir_subject_t* arg) {
    if (!arg) return 0;
    if (arg->t == LIR_VARIABLE) {
        set_add(use, arg->storage.var.v_id);
        return 1;
    }

    if (arg->t == LIR_ARGLIST) {
        foreach (lir_subject_t* it, &arg->storage.list.h) {
            if (it->t == LIR_VARIABLE) {
                set_add(use, it->storage.var.v_id);
            }
        }
    }

    return 1;
}

int LIR_drop_unused_variables(cfg_ctx_t* cctx) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        set_t use, def;
        map_t def_trg;
        map_init(&def_trg, MAP_NO_CMP);
        set_init(&use, SET_NO_CMP);
        set_init(&def, SET_NO_CMP);
        foreach (cfg_block_t* bb, &fb->blocks) {
            iterate_lir_instructions (bb) {
                if (!lh->unused) {
                    iterate_lir_args (lir_subject_t* arg, lh, LIR_is_writeop(lh->op)) {
                        _mark_used_var(&use, arg);
                    }

                    switch (lh->op) {
                        case LIR_TF64: case LIR_TF32: 
                        case LIR_TI64: case LIR_TI32: case LIR_TI16: case LIR_TI8: 
                        case LIR_TU64: case LIR_TU32: case LIR_TU16: case LIR_TU8:
                        case LIR_iMOV: case LIR_aMOV: {                        
                            if (lh->farg && lh->farg->t == LIR_VARIABLE) {
                                set_add(&def, lh->farg->storage.var.v_id);
                                map_put(&def_trg, lh->farg->storage.var.v_id, lh);
                            }

                            break;
                        }
                        default: break;
                    }
                }
            }
        }

        set_minus_set(&def, &use);
        set_foreach (symbol_id_t unused, &def) {
            lir_block_t* dst;
            if (map_get(&def_trg, unused, (void**)&dst)) dst->unused = 1;
        }

        map_free(&def_trg);
        set_free(&use);
        set_free(&def);
    }

    return 1;
}

/* Replace readable operands with their propagated copy when a mapping exists.
Params:
    - `l` - LIR block to rewrite.
    - `gen` - Map from variable/register keys to copied subjects.
    - `t` - Subject type to replace.

Returns 1 if succeeds */
static int _replace_with_copy(lir_block_t* l, map_t* gen, lir_subject_type_t t) {
    iterate_ref_lir_args (lir_subject_t** curr, l, LIR_is_writeop(l->op)) {
        lir_subject_t* dst;
        if (
            (*curr)->t == t && 
            map_get(
                gen, 
                t == LIR_VARIABLE ? (*curr)->storage.var.v_id : LIR_format_register((*curr)->storage.reg.reg, 1), 
                (void**)&dst
            )
        ) {
            if ((*curr)->home == l) LIR_unload_subject(*curr);
            *curr = LIR_copy_subject(dst);
            (*curr)->home = l;
        }
    }


    return 1;
}

/* Check whether a subject is a non-global variable.
Params:
    - `s` - LIR subject to inspect.
    - `smt` - Symtable.

Returns 1 for a local variable, otherwise 0 */
static inline int _is_local_variable(lir_subject_t* s, sym_table_t* smt) {
    if (!s || s->t != LIR_VARIABLE) return 0;
    variable_info_t vi;
    return VRTB_get_info_id(s->storage.var.v_id, &vi, &smt->v) && !vi.vfs.glob;
}

/* Check whether an instruction can generate a copy-propagation fact.
Only local destinations are accepted; address-taken variables are skipped.
Params:
    - `lh` - LIR instruction to inspect.
    - `smt` - Symtable.
    - `addr_taken` - Set of variables whose address is taken.
    - `src_size` - Optional output for propagated source size.

Returns 1 if the instruction is a safe copy candidate, otherwise 0 */
static int _is_copy_candidate(lir_block_t* lh, sym_table_t* smt, set_t* addr_taken, long* src_size) {
    if (!lh || !_is_local_variable(lh->farg, smt)) return 0;
    if (set_has(addr_taken, (void*)lh->farg->storage.var.v_id)) return 0;
    long sarg_size = lh->sarg ? lh->sarg->size : 0;
    switch (lh->op) {
        case LIR_TF64: case LIR_TF32:
        case LIR_TI64: case LIR_TI32: case LIR_TI16: case LIR_TI8:
        case LIR_TU64: case LIR_TU32: case LIR_TU16: case LIR_TU8: {
            if (!lh->sarg || (lh->sarg->t != LIR_NUMBER && lh->sarg->t != LIR_CONSTVAL)) return 0;
            sarg_size = lh->farg->size;
            break;
        }
        case LIR_iMOV: {
            if (!lh->sarg) return 0;
            if (
                lh->sarg->t == LIR_VARIABLE &&
                (
                    !_is_local_variable(lh->sarg, smt) ||
                    set_has(addr_taken, (void*)lh->sarg->storage.var.v_id)
                )
            ) return 0;
            break;
        }
        default: return 0;
    }

    if (src_size) *src_size = sarg_size;
    return 1;
}

/* Store a copied subject in the propagation map.
Replaces and unloads any previous value with the same key.
Params:
    - `m` - Propagation map.
    - `key` - Variable or register key.
    - `src` - Subject to copy.
    - `src_size` - Size to assign to the stored copy.

Returns 1 if succeeds */
static int _map_put_subject_copy(map_t* m, long key, lir_subject_t* src, long src_size) {
    lir_subject_t* old = NULL;
    if (map_get(m, key, (void**)&old)) LIR_unload_subject(old);
    lir_subject_t* copy = LIR_copy_subject(src);
    if (!copy) return 0;
    copy->size = src_size;
    return map_put(m, key, copy);
}

/* Deep-copy a propagation map with LIR subjects as values.
Params:
    - `dst` - Destination map to initialize and fill.
    - `src` - Source map.

Returns 1 if succeeds */
static int _map_copy_subjects(map_t* dst, map_t* src) {
    map_init(dst, src->cmp);
    for (long i = 0; i < src->capacity; i++) {
        if (!src->entries[i].used) continue;
        lir_subject_t* copy = LIR_copy_subject((lir_subject_t*)src->entries[i].value);
        if (!copy || !map_put(dst, src->entries[i].key, copy)) return 0;
    }

    return 1;
}

/* Replace destination map contents with a deep copy of the source map.
Params:
    - `dst` - Destination map to overwrite.
    - `src` - Source map.

Returns 1 if succeeds */
static inline int _map_assign_subjects(map_t* dst, map_t* src) {
    map_free_force_op(dst, (int (*)(void*))LIR_unload_subject);
    return _map_copy_subjects(dst, src);
}

/* Drop all propagation facts from a map and keep it initialized.
Params:
    - `m` - Propagation map to clear.

Returns 1 if succeeds */
static inline int _map_clear_subjects(map_t* m) {
    map_free_force_op(m, (int (*)(void*))LIR_unload_subject);
    return map_init(m, MAP_NO_CMP);
}

/* Compare two propagation maps by key set and subject equality.
Params:
    - `a` - First map.
    - `b` - Second map.

Returns 1 if maps contain equal subjects for equal keys, otherwise 0 */
static int _map_subjects_equal(map_t* a, map_t* b) {
    if (a->size != b->size) return 0;
    for (long i = 0; i < a->capacity; i++) {
        if (!a->entries[i].used) continue;
        lir_subject_t* bv = NULL;
        if (
            !map_get(b, a->entries[i].key, (void**)&bv) ||
            !LIR_subj_equals((lir_subject_t*)a->entries[i].value, bv)
        ) return 0;
    }

    return 1;
}

/* Build the meet of two propagation maps.
Keeps only facts present in both maps with equal copied subjects.
Params:
    - `dst` - Destination map to initialize and fill.
    - `a` - First source map.
    - `b` - Second source map.

Returns 1 if succeeds */
static int _map_intersect_subjects(map_t* dst, map_t* a, map_t* b) {
    map_init(dst, MAP_NO_CMP);
    for (long i = 0; i < a->capacity; i++) {
        if (!a->entries[i].used) continue;
        lir_subject_t* bv = NULL;
        if (
            map_get(b, a->entries[i].key, (void**)&bv) &&
            LIR_subj_equals((lir_subject_t*)a->entries[i].value, bv)
        ) {
            lir_subject_t* copy = LIR_copy_subject((lir_subject_t*)a->entries[i].value);
            if (!copy || !map_put(dst, a->entries[i].key, copy)) return 0;
        }
    }

    return 1;
}

/* Remove all propagation facts invalidated by a variable write.
Kills the variable itself and every fact that reads from it.
Params:
    - `m` - Propagation map to update.
    - `v_id` - Written variable id.

Returns 1 if succeeds */
static int _map_kill_variable(map_t* m, symbol_id_t v_id) {
    lir_subject_t* old = NULL;
    if (map_get(m, v_id, (void**)&old)) {
        map_remove(m, v_id);
        LIR_unload_subject(old);
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (long i = 0; i < m->capacity; i++) {
            if (!m->entries[i].used) continue;
            lir_subject_t* src = (lir_subject_t*)m->entries[i].value;
            if (src->t == LIR_VARIABLE && src->storage.var.v_id == v_id) {
                long key = m->entries[i].key;
                map_remove(m, key);
                LIR_unload_subject(src);
                changed = 1;
                break;
            }
        }
    }

    return 1;
}

/* Build a map key for a physical register.
Params:
    - `reg` - Register to normalize.

Returns normalized register key */
static inline long _register_key(lir_registers_t reg) {
    return LIR_format_register(reg, 1);
}

/* Check whether two register subjects address the same physical register family.
Params:
    - `a` - First subject.
    - `b` - Second subject.

Returns 1 if both subjects are aliases of the same register */
static inline int _same_register_subject(lir_subject_t* a, lir_subject_t* b) {
    return (
        a && b && a->t == LIR_REGISTER && b->t == LIR_REGISTER &&
        (_register_key(a->storage.reg.reg) == _register_key(b->storage.reg.reg))
    );
}

/* Check whether a subject is a stack/frame pointer register.
Params:
    - `s` - Subject to inspect.

Returns 1 for RSP/RBP aliases, otherwise 0 */
static inline int _is_reserved_stack_register(lir_subject_t* s) {
    if (!s || s->t != LIR_REGISTER) return 0;
    long key = _register_key(s->storage.reg.reg);
    return key == _register_key(RSP) || key == _register_key(RBP);
}

/* Remove all propagation facts invalidated by a register write.
Kills the register itself and every fact that reads from it.
Params:
    - `m` - Propagation map to update.
    - `reg` - Written register.

Returns 1 if succeeds */
static int _map_kill_register(map_t* m, lir_registers_t reg) {
    long reg_key = _register_key(reg);

    lir_subject_t* old = NULL;
    if (map_get(m, reg_key, (void**)&old)) {
        map_remove(m, reg_key);
        LIR_unload_subject(old);
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (long i = 0; i < m->capacity; i++) {
            if (!m->entries[i].used) continue;
            lir_subject_t* src = (lir_subject_t*)m->entries[i].value;
            if (src->t == LIR_REGISTER && _register_key(src->storage.reg.reg) == reg_key) {
                long key = m->entries[i].key;
                map_remove(m, key);
                LIR_unload_subject(src);
                changed = 1;
                break;
            }
        }
    }

    return 1;
}

/* Apply copy-propagation transfer rules for one instruction.
Params:
    - `lh` - LIR instruction.
    - `state` - Current propagation state.
    - `smt` - Symtable.
    - `addr_taken` - Set of variables whose address is taken.

Returns 1 if succeeds */
static int _transfer_instruction(lir_block_t* lh, map_t* state, sym_table_t* smt, set_t* addr_taken) {
    if (!lh || lh->unused) return 1;
    if (LIR_is_writeop(lh->op) && lh->farg && lh->farg->t == LIR_VARIABLE) {
        _map_kill_variable(state, lh->farg->storage.var.v_id);
    }

    long src_size = 0;
    if (_is_copy_candidate(lh, smt, addr_taken, &src_size)) {
        _map_put_subject_copy(state, lh->farg->storage.var.v_id, lh->sarg, src_size);
    }

    return 1;
}

/* Compute the output propagation state for a basic block.
Params:
    - `bb` - Basic block to transfer through.
    - `in` - Input propagation state.
    - `out` - Output propagation state to overwrite.
    - `smt` - Symtable.
    - `addr_taken` - Set of variables whose address is taken.

Returns 1 if succeeds */
static int _transfer_block(cfg_block_t* bb, map_t* in, map_t* out, sym_table_t* smt, set_t* addr_taken) {
    map_t tmp;
    _map_copy_subjects(&tmp, in);
    iterate_lir_instructions (bb) {
        _transfer_instruction(lh, &tmp, smt, addr_taken);
    }

    _map_assign_subjects(out, &tmp);
    map_free_force_op(&tmp, (int (*)(void*))LIR_unload_subject);
    return 1;
}

/* Compute block-local copy gen/kill summaries.
Params:
    - `bb` - Basic block to inspect.
    - `smt` - Symtable.
    - `addr_taken` - Set of variables whose address is taken.

Returns 1 if succeeds */
static int _compute_block_copy_sets(cfg_block_t* bb, sym_table_t* smt, set_t* addr_taken) {
    set_free(&bb->copy_gen);
    set_init(&bb->copy_gen, SET_NO_CMP);
    set_free(&bb->copy_kill);
    set_init(&bb->copy_kill, SET_NO_CMP);

    iterate_lir_instructions (bb) {
        if (lh->unused) continue;
        if (LIR_is_writeop(lh->op) && lh->farg && lh->farg->t == LIR_VARIABLE) {
            set_add(&bb->copy_kill, (void*)lh->farg->storage.var.v_id);
            set_remove(&bb->copy_gen, (void*)lh->farg->storage.var.v_id);
        }

        if (
            _is_copy_candidate(lh, smt, addr_taken, NULL)
        ) set_add(&bb->copy_gen, (void*)lh->farg->storage.var.v_id);
    }

    return 1;
}

/* Collect variables whose address is materialized in a function.
Params:
    - `fb` - Function CFG.
    - `addr_taken` - Output set to initialize and fill.

Returns 1 if succeeds */
static int _collect_address_taken_variables(cfg_func_t* fb, set_t* addr_taken) {
    set_init(addr_taken, SET_NO_CMP);
    foreach (cfg_block_t* bb, &fb->blocks) {
        iterate_lir_instructions (bb) {
            if (
                !lh->unused &&
                lh->op == LIR_REF && lh->sarg && lh->sarg->t == LIR_VARIABLE
            ) set_add(addr_taken, (void*)lh->sarg->storage.var.v_id);

            if (
                !lh->unused && 
                lh->op == LIR_aMOV && lh->farg && lh->farg->t == LIR_VARIABLE
            ) set_add(addr_taken, (void*)lh->farg->storage.var.v_id);
        }
    }

    return 1;
}

/* Build a block input state from predecessor output states.
The meet operation keeps only facts agreed on by every predecessor.
Params:
    - `dst` - Destination input state to initialize and fill.
    - `bb` - Basic block whose predecessors are read.
    - `out_by_block` - Map from block id to output state.

Returns 1 if succeeds */
static int _build_block_in(map_t* dst, cfg_block_t* bb, map_t* out_by_block) {
    map_init(dst, MAP_NO_CMP);
    int first = 1;
    set_foreach (cfg_block_t* pred, &bb->pred) {
        map_t* pred_out = NULL;
        if (!map_get(out_by_block, pred->id, (void**)&pred_out)) continue;
        if (first) {
            map_free(dst);
            _map_copy_subjects(dst, pred_out);
            first = 0;
            continue;
        }

        map_t tmp;
        _map_intersect_subjects(&tmp, dst, pred_out);
        map_free_force_op(dst, (int (*)(void*))LIR_unload_subject);
        *dst = tmp;
    }

    return 1;
}

/* Rewrite readable operands in a basic block using available copy facts.
Params:
    - `bb` - Basic block to rewrite.
    - `in` - Input propagation state.
    - `smt` - Symtable.
    - `addr_taken` - Set of variables whose address is taken.

Returns 1 if succeeds */
static int _rewrite_block(cfg_block_t* bb, map_t* in, sym_table_t* smt, set_t* addr_taken) {
    map_t state;
    _map_copy_subjects(&state, in);
    iterate_lir_instructions (bb) {
        if (!lh->unused) {
            if (
                LIR_is_readop(lh->op) &&
                lh->op != LIR_aMOV &&
                lh->op != LIR_REF
            ) _replace_with_copy(lh, &state, LIR_VARIABLE);
            _transfer_instruction(lh, &state, smt, addr_taken);
        }
    }

    map_free_force_op(&state, (int (*)(void*))LIR_unload_subject);
    return 1;
}

/* Check whether an instruction clobbers register facts globally.
Params:
    - `op` - LIR operation to inspect.

Returns 1 when all register facts must be dropped */
static inline int _register_facts_clobbered_by_op(lir_operation_t op) {
    switch (op) {
        case LIR_FCLL: case LIR_ECLL: case LIR_SYSC:
        case LIR_RAW:  case LIR_RAWASM: return 1;
        default:                        return 0;
    }
}

/* Check whether an instruction writes to its first argument register.
Params:
    - `op` - LIR operation to inspect.

Returns 1 when farg register facts must be killed */
static inline int _register_op_writes_farg(lir_operation_t op) {
    switch (op) {
        case LIR_SETL: case LIR_SETG: case LIR_STLE:
        case LIR_STGE: case LIR_SETE: case LIR_STNE:
        case LIR_SETB: case LIR_SETA: case LIR_STBE:
        case LIR_STAE: return 1;
        default:       return LIR_is_writeop(op);
    }
}

/* Check whether an instruction can generate a register copy fact.
Params:
    - `lh` - LIR instruction to inspect.
    - `src_size` - Optional output for propagated source size.

Returns 1 if the instruction is a safe register copy candidate */
static int _is_register_copy_candidate(lir_block_t* lh, long* src_size) {
    if (
        !lh || lh->unused ||
        (lh->op != LIR_iMOV && lh->op != LIR_phiMOV) ||
        !lh->farg || lh->farg->t != LIR_REGISTER ||
        !lh->sarg || _is_reserved_stack_register(lh->farg)
    ) return 0;
    switch (lh->sarg->t) {
        case LIR_REGISTER: {
            if (
                _same_register_subject(lh->farg, lh->sarg) ||
                _is_reserved_stack_register(lh->sarg)      ||
                (lh->farg->size != lh->sarg->size)
            ) return 0;
            if (src_size) *src_size = lh->sarg->size;
            return 1;
        }
        case LIR_CONSTVAL:
        case LIR_NUMBER: {
            if (src_size) *src_size = lh->farg->size;
            return 1;
        }
        default: return 0;
    }
}

/* Apply register copy-propagation transfer rules for one instruction.
Params:
    - `lh` - LIR instruction.
    - `state` - Current propagation state.

Returns 1 if succeeds */
static int _transfer_register_instruction(lir_block_t* lh, map_t* state) {
    if (!lh || lh->unused) return 1;

    if (_register_facts_clobbered_by_op(lh->op)) {
        _map_clear_subjects(state);
        return 1;
    }

    if (
        _register_op_writes_farg(lh->op) && 
        lh->farg && lh->farg->t == LIR_REGISTER
    ) _map_kill_register(state, lh->farg->storage.reg.reg);
    
    if (
        lh->op == LIR_XCHG && 
        lh->sarg && lh->sarg->t == LIR_REGISTER
    ) _map_kill_register(state, lh->sarg->storage.reg.reg);
    
    if (lh->op == LIR_CDQ || lh->op == LIR_CQO) {
        _map_kill_register(state, RDX);
    }
    
    if (
        lh->op == LIR_DIV  || 
        lh->op == LIR_iDIV || 
        lh->op == LIR_iMOD
    ) {
        _map_kill_register(state, RAX);
        _map_kill_register(state, RDX);
    }

    long src_size = 0;
    if (_is_register_copy_candidate(lh, &src_size)) {
        _map_put_subject_copy(state, _register_key(lh->farg->storage.reg.reg), lh->sarg, src_size);
    }

    return 1;
}

/* Compute the output register propagation state for a basic block.
Params:
    - `bb` - Basic block to transfer through.
    - `in` - Input propagation state.
    - `out` - Output propagation state to overwrite.

Returns 1 if succeeds */
static int _transfer_register_block(cfg_block_t* bb, map_t* in, map_t* out) {
    map_t tmp;
    _map_copy_subjects(&tmp, in);
    iterate_lir_instructions (bb) {
        _transfer_register_instruction(lh, &tmp);
    }

    _map_assign_subjects(out, &tmp);
    map_free_force_op(&tmp, (int (*)(void*))LIR_unload_subject);
    return 1;
}

/* Rewrite a move source register using the current propagation state.
Params:
    - `lh` - LIR instruction to rewrite.
    - `state` - Current propagation state.

Returns 1 if succeeds */
static int _rewrite_register_mov_source(lir_block_t* lh, map_t* state) {
    if (
        !lh || lh->unused ||
        (lh->op != LIR_iMOV && lh->op != LIR_phiMOV) ||
        !lh->farg || !lh->sarg || lh->sarg->t != LIR_REGISTER ||
        lh->farg->size != lh->sarg->size
    ) return 1;
    if (_same_register_subject(lh->farg, lh->sarg)) return 1;

    lir_subject_t* copy = NULL;
    if (!map_get(state, _register_key(lh->sarg->storage.reg.reg), (void**)&copy)) return 1;
    if (copy->t == LIR_REGISTER) return 1;

    if (lh->sarg->home == lh) LIR_unload_subject(lh->sarg);
    lh->sarg = LIR_copy_subject(copy);
    lh->sarg->home = lh;
    return 1;
}

/* Rewrite register copies in a basic block using available register facts.
Params:
    - `bb` - Basic block to rewrite.
    - `in` - Input propagation state.

Returns 1 if succeeds */
static int _rewrite_register_block(cfg_block_t* bb, map_t* in) {
    map_t state;
    _map_copy_subjects(&state, in);
    iterate_lir_instructions (bb) {
        if (!lh->unused) {
            _rewrite_register_mov_source(lh, &state);
            _transfer_register_instruction(lh, &state);
        }
    }

    map_free_force_op(&state, (int (*)(void*))LIR_unload_subject);
    return 1;
}

static void _prepare_in_out(cfg_func_t* fb, sym_table_t* smt, set_t* addr_taken, map_t* in_by_block, map_t* out_by_block) {
    if (addr_taken) _collect_address_taken_variables(fb, addr_taken);
    map_init(in_by_block, MAP_NO_CMP);
    map_init(out_by_block, MAP_NO_CMP);
    foreach (cfg_block_t* bb, &fb->blocks) {
        if (smt) _compute_block_copy_sets(bb, smt, addr_taken);
        map_t* in  = (map_t*)mm_malloc(sizeof(map_t));
        map_t* out = (map_t*)mm_malloc(sizeof(map_t));
        map_init(in, MAP_NO_CMP);
        map_init(out, MAP_NO_CMP);
        map_put(in_by_block, bb->id, in);
        map_put(out_by_block, bb->id, out);
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        foreach (cfg_block_t* bb, &fb->blocks) {
            map_t new_in;
            _build_block_in(&new_in, bb, out_by_block);

            map_t *old_in = NULL, *old_out = NULL;
            map_get(in_by_block, bb->id, (void**)&old_in);
            map_get(out_by_block, bb->id, (void**)&old_out);
            if (!_map_subjects_equal(old_in, &new_in)) {
                _map_assign_subjects(old_in, &new_in);
                changed = 1;
            }

            map_t new_out;
            map_init(&new_out, MAP_NO_CMP);
            if (addr_taken) _transfer_block(bb, old_in, &new_out, smt, addr_taken);
            else            _transfer_register_block(bb, old_in, &new_out);
            if (!_map_subjects_equal(old_out, &new_out)) {
                _map_assign_subjects(old_out, &new_out);
                changed = 1;
            }

            map_free_force_op(&new_in, (int (*)(void*))LIR_unload_subject);
            map_free_force_op(&new_out, (int (*)(void*))LIR_unload_subject);
        }
    }
}

static void _release_in_out(set_t* addr_taken, map_t* in_by_block, map_t* out_by_block) {
    map_foreach (map_t* in, in_by_block) {
        map_free_force_op(in, (int (*)(void*))LIR_unload_subject);
        mm_free(in);
    }

    map_foreach (map_t* out, out_by_block) {
        map_free_force_op(out, (int (*)(void*))LIR_unload_subject);
        mm_free(out);
    }
    
    map_free(in_by_block);
    map_free(out_by_block);
    if (addr_taken) set_free(addr_taken);
}

int LIR_variable_copy_propagation(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        set_t addr_taken;
        map_t in_by_block, out_by_block;
        _prepare_in_out(fb, smt, &addr_taken, &in_by_block, &out_by_block);

        foreach (cfg_block_t* bb, &fb->blocks) {
            map_t* in = NULL;
            if (map_get(&in_by_block, bb->id, (void**)&in)) {
                _rewrite_block(bb, in, smt, &addr_taken);
            }
        }

        _release_in_out(&addr_taken, &in_by_block, &out_by_block);
    }
    
    return 1;
}

int LIR_register_copy_propagation(cfg_ctx_t* cctx) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        map_t in_by_block, out_by_block;
        _prepare_in_out(fb, NULL, NULL, &in_by_block, &out_by_block);

        foreach (cfg_block_t* bb, &fb->blocks) {
            map_t* in = NULL;
            if (map_get(&in_by_block, bb->id, (void**)&in)) {
                _rewrite_register_block(bb, in);
            }
        }

        _release_in_out(NULL, &in_by_block, &out_by_block);
    }

    return 1;
}
