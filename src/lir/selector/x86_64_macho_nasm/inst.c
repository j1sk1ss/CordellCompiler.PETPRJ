#include <lir/selector/x86_64_macho_nasm.h>
// TODO: Complete AVX support

/* Insert block before 'pos' block with the block entry update.
Params:
    - `bb` - Source block.
    - `b` - New block for an insertion process.
    - `pos` - Position for an insert */
static inline void _insert_instruction_before(cfg_block_t* bb, lir_block_t* b, lir_block_t* pos) {
    if (!b) return;
    if (bb->lmap.entry == pos) bb->lmap.entry = b;
    LIR_insert_block_before(b, pos);
}

/* Insert block after 'pos' block with the block exit update.
Params:
    - `bb` - Source block.
    - `b` - New block for an insertion process.
    - `pos` - Position for an insert */
static inline void _insert_instruction_after(cfg_block_t* bb, lir_block_t* b, lir_block_t* pos) {
    if (!b) return;
    if (bb->lmap.exit == pos) bb->lmap.exit = b;
    LIR_insert_block_after(b, pos);
}

static inline int _is_external_global(lir_subject_t* s, variable_info_t* out, sym_table_t* smt) {
    variable_info_t vi;
    if (
        !s || (s->t != LIR_VARIABLE && s->t != LIR_GLVARIABLE) ||
        !VRTB_get_info_id(s->storage.var.v_id, &vi, &smt->v)   ||
        !vi.vfs.ext || !vi.vfs.glob
    ) return 0;
    if (out) str_memcpy(out, &vi, sizeof(vi));
    return 1;
}

static inline int _external_value_size(lir_subject_t* s, variable_info_t* vi) {
    if (s && s->size > 0)       return s->size;
    if (vi && vi->vmi.size > 0) return vi->vmi.size;
    return CONF_get_full_bytness();
}

static inline lir_subject_t* _create_tmp_var(token_type_t type, basic_object_info_t flags, int size, int dsize, sym_table_t* smt) {
    lir_subject_t* res = LIR_SUBJ_VAR(VRTB_add_info(NULL, type, NO_SYMBOL_ID, flags, &smt->v), size);
    res->dsize = dsize;
    return res;
}

static lir_subject_t* _external_global_addr(cfg_block_t* bb, lir_block_t* pos, lir_subject_t* s, sym_table_t* smt) {
    variable_info_t vi;
    if (!_is_external_global(s, &vi, smt)) return s;
    lir_subject_t* addr = _create_tmp_var(TMP_U64_TYPE_TOKEN, (basic_object_info_t){ .ptr = 1 }, CONF_get_full_bytness(), _external_value_size(s, &vi), smt);
    _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, addr, s, NULL), pos);
    return addr;
}

static lir_subject_t* _external_global_value(cfg_block_t* bb, lir_block_t* pos, lir_subject_t* s, sym_table_t* smt) {
    variable_info_t vi;
    if (!_is_external_global(s, &vi, smt)) return s;
    int size = _external_value_size(s, &vi);
    lir_subject_t* addr = _external_global_addr(bb, pos, s, smt);
    lir_subject_t* val  = _create_tmp_var(vi.type, (basic_object_info_t){ .ptr = vi.vfs.ptr }, size, s->dsize, smt);
    _insert_instruction_before(bb, LIR_create_block(LIR_GDREF, val, addr, NULL), pos);
    return val;
}

static inline int _materialize_external_read(cfg_block_t* bb, lir_block_t* lh, lir_subject_t** s, sym_table_t* smt) {
    if (!s) return 0;
    if (!_is_external_global(*s, NULL, smt)) return 0;
    *s = _external_global_value(bb, lh, *s, smt);
    return 1;
}

static inline int _materialize_external_addr(cfg_block_t* bb, lir_block_t* lh, lir_subject_t** s, sym_table_t* smt) {
    if (!s || !_is_external_global(*s, NULL, smt)) return 0;
    *s = _external_global_addr(bb, lh, *s, smt);
    return 1;
}

static void _materialize_external_globals(cfg_block_t* bb, lir_block_t* lh, sym_table_t* smt) {
    if (lh->op == LIR_REF) {
        if (_materialize_external_addr(bb, lh, &lh->sarg, smt)) lh->op = LIR_iMOV;
        return;
    }

    if (lh->op == LIR_REF_GDREF && _materialize_external_read(bb, lh, &lh->sarg, smt)) {
        lh->op = LIR_iMOV;
        return;
    }

    if (lh->op == LIR_STSARG || lh->op == LIR_STFARG) {
        _materialize_external_read(bb, lh, &lh->farg, smt);
        return;
    }

    variable_info_t vi;
    if (
        (
            lh->op == LIR_iMOV   || lh->op == LIR_aMOV || 
            lh->op == LIR_phiMOV || lh->op == LIR_fMOV || 
            lh->op == LIR_fMVf
        ) && _is_external_global(lh->farg, &vi, smt)
    ) {
        _materialize_external_read(bb, lh, &lh->sarg, smt);
        int size = _external_value_size(lh->farg, &vi);
        lh->farg = _external_global_addr(bb, lh, lh->farg, smt);
        lh->farg->dsize = size;
        lh->op = LIR_LDREF;
        return;
    }

    if (lh->op != LIR_VRUSE && LIR_is_readop(lh->op)) {
        iterate_ref_lir_args (lir_subject_t** s, lh, LIR_is_writeop(lh->op)) {
            _materialize_external_read(bb, lh, s, smt);
        }
    }
}

typedef struct {
    lir_registers_t reg;
    int             off;
} abi_argument_t;

/* Generate the information which will tell where we should put a value for a function.
Params:
    - `index` - Argument index.
    - `s` - Target value which will be placed to a function.
    - `out` - Output information placeholder.
    - `smt` - Symtable.

Returns 1 if this is a register value, otherwise 0 */
static int _get_abi_argument(int index, lir_subject_t* s, abi_argument_t* out, func_info_t* fi, sym_table_t* smt) {
    int dec_abi_regs[]  = { RDI,  RSI,  RDX,  RCX,  R8,   R9 };
    int simd_abi_regs[] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7 };

    int is_float = 0;
    switch (s->t) {
        case LIR_VARIABLE: is_float = x86_64_macho_nasm_is_simd_type(s, smt); break;
        case LIR_NUMBER:   is_float = s->storage.num.is_float;                break;
        default: break;
    }

    if (fi->flags.vargs && !fi->flags.abi) {
        out->off = (index + !fi->flags.naked + 1) * -8;
        return 0;
    }

    int *regs = NULL, regs_count = 0;
    if (!is_float) {
        regs       = dec_abi_regs;
        regs_count = (int)(sizeof(dec_abi_regs) / sizeof(RAX));
    }
    else {
        regs       = simd_abi_regs;
        regs_count = (int)(sizeof(simd_abi_regs) / sizeof(XMM0));
    }

    if (index >= regs_count) {
        out->off = (index - regs_count + !fi->flags.naked + 1) * -8;
        return 0;
    }
    
    out->reg = regs[index];
    return 1;
}

/* Count fixed function arguments.
Variadic marker arguments are not counted because they do not occupy a
regular named-argument slot.
Params:
    - `f_id` - Function id.
    - `smt` - Symtable.

Returns count of presented non-variadic arguments */
static int _count_presented_args(symbol_id_t f_id, sym_table_t* smt) {
    func_info_t fi;
    if (!FNTB_get_info_id(f_id, &fi, &smt->f)) return 0;
    int res = 0;
    fn_iterate_args (&fi) {
        if (arg->t && arg->t->t_type != VAR_ARGUMENTS_TOKEN) res++;
    }

    return res;
}

/* Find callee metadata for the call that consumes an argument setup block.
Params:
    - `arg` - Argument setup instruction.
    - `bb` - Basic block that owns the instruction.
    - `out` - Output function information.
    - `smt` - Symtable.

Returns 1 if a following named call was found and resolved, otherwise 0. */
static int _get_call_info(lir_block_t* arg, cfg_block_t* bb, func_info_t* out, sym_table_t* smt) {
    for (lir_block_t* curr = arg->next; curr; curr = LIR_get_next(curr, bb->lmap.exit, 1)) {
        if (curr->op != LIR_FCLL && curr->op != LIR_ECLL) continue;
        if (!curr->farg || curr->farg->t != LIR_FNAME) return 0;
        return FNTB_get_info_id(curr->farg->storage.str.sid, out, &smt->f);
    }

    return 0;
}

typedef struct {
    int clean_stack;
    set_t callee_restored_blocks;
} lir_translate_ctx_t;

static cfg_dfs_action_t _instruction_selection_block(
    cfg_block_t* bb, long pred, cfg_func_t* fb, func_info_t* fi, queue_t* dirty_regs, lir_translate_ctx_t* ctx, sym_table_t* smt
) {
    (void)pred;
    iterate_lir_instructions (bb) {
        _materialize_external_globals(bb, lh, smt);
        switch (lh->op) {
            case LIR_STSARG: {
                int sys_regs[] = { RAX, RDI, RSI, RDX, R10, R8, R9 };
                if (lh->sarg->storage.cnst.value >= (long)(sizeof(sys_regs) / sizeof(RAX))) break;
                if (sys_regs[lh->sarg->storage.cnst.value] != RAX) {
                    _insert_instruction_before(
                        bb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(sys_regs[lh->sarg->storage.cnst.value], CONF_get_full_bytness()), NULL, NULL), 
                        lh
                    );
                    queue_push(dirty_regs, (void*)((long)sys_regs[lh->sarg->storage.cnst.value]));
                }

                lir_subject_t* nfarg = x86_64_macho_nasm_create_tmp(sys_regs[lh->sarg->storage.cnst.value], lh->farg, smt, CONF_get_full_bytness());
                LIR_unload_subject(lh->sarg);
                lh->op   = LIR_aMOV;
                lh->sarg = lh->farg;
                lh->farg = nfarg;
                break;
            }
            case LIR_REF_ARGS: {
                lh->op   = LIR_REF;
                lh->sarg = LIR_SUBJ_OFF(RBP, (!fi->flags.naked + _count_presented_args(fb->f_id, smt) + 1) * -8, CONF_get_full_bytness());
                break;
            }
            case LIR_ECLL:
            case LIR_FCLL: {
                if (ctx->clean_stack) {
                    _insert_instruction_after(bb, LIR_create_block(LIR_iADD, LIR_SUBJ_REG(RSP, CONF_get_full_bytness()), LIR_SUBJ_REG(RSP, CONF_get_full_bytness()), LIR_SUBJ_CONST(ctx->clean_stack)), lh);
                    ctx->clean_stack = 0;
                }
                __attribute__ ((fallthrough));
            }
            case LIR_SYSC: {
                if (lh->op == LIR_SYSC) { /* https://stackoverflow.com/questions/50571275/why-does-a-syscall-clobber-rcx-and-r11 */
                    _insert_instruction_before(bb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(RCX, CONF_get_full_bytness()), NULL, NULL), lh);
                    queue_push(dirty_regs, (void*)((long)RCX));
                    _insert_instruction_before(bb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(R11, CONF_get_full_bytness()), NULL, NULL), lh);
                    queue_push(dirty_regs, (void*)((long)R11));
                }

                long dirty;
                while (queue_pop(dirty_regs, (void**)&dirty)) {
                    _insert_instruction_after(bb, LIR_create_block(LIR_POP, LIR_SUBJ_REG(dirty, CONF_get_full_bytness()), NULL, NULL), lh);
                }

                break;
            }
            case LIR_STARGLD: {
                lir_subject_t* src;
                switch (lh->sarg->storage.cnst.value) {
                    case 0: {
                        src = x86_64_macho_nasm_create_tmp(RDI, lh->farg, smt, -1);
                        lh->op = LIR_iMOV; 
                        break;
                    }
                    default: {
                        src = x86_64_macho_nasm_create_tmp(RSI, lh->farg, smt, -1);
                        lh->op = LIR_REF_GDREF;  
                        break;
                    }
                }

                LIR_unload_subject(lh->sarg);
                lh->sarg = src;
                break;
            }
            case LIR_STFARG: {
                func_info_t callee;
                if (!_get_call_info(lh, bb, &callee, smt)) callee = *fi;
                abi_argument_t target;
                if (!_get_abi_argument(lh->sarg->storage.cnst.value, lh->farg, &target, &callee, smt)) {
                    lh->op = LIR_PUSH;
                    ctx->clean_stack += 8;
                }
                else {
                    lir_subject_t* nfarg = x86_64_macho_nasm_create_tmp(target.reg, lh->farg, smt, -1);
                    LIR_insert_block_before(LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(target.reg, CONF_get_full_bytness()), NULL, NULL), lh);
                    queue_push(dirty_regs, (void*)((long)target.reg));
                    LIR_unload_subject(lh->sarg);
                    lh->op   = LIR_aMOV;
                    lh->sarg = lh->farg;
                    lh->farg = nfarg;
                }
                
                break;
            }
            case LIR_LOADFARG: {
                abi_argument_t target;
                lir_subject_t* nfarg;
                if (
                    _get_abi_argument(lh->sarg->storage.cnst.value, lh->farg, &target, fi, smt)
                ) nfarg = x86_64_macho_nasm_create_tmp(target.reg, lh->farg, smt, -1);
                else nfarg = LIR_SUBJ_OFF(RBP, target.off, lh->farg->size);
                LIR_unload_subject(lh->sarg);
                lh->op   = LIR_phiMOV;
                lh->sarg = nfarg;
                break;
            }
            case LIR_LOADFRET: {
                lh->op   = LIR_iMOV;
                lh->sarg = x86_64_macho_nasm_create_tmp(RAX, lh->farg, smt, -1);
                break;
            }
            case LIR_NOT: {
                lir_subject_t* a   = x86_64_macho_nasm_create_tmp(RAX, lh->sarg, smt, -1);
                lir_subject_t* res = LIR_SUBJ_REG(AL, 1);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->sarg, NULL), lh);
                _insert_instruction_before(bb, LIR_create_block(LIR_TST, a, a, NULL), lh);
                _insert_instruction_before(bb, LIR_create_block(LIR_SETE, res, NULL, NULL), lh);
                lh->op = LIR_iMOV;
                lh->sarg = res;
                break;
            }
            case LIR_NEG: {
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, LIR_copy_subject(lh->farg), lh->sarg, NULL), lh);
                lh->sarg = LIR_copy_subject(lh->farg);
                break;
            }
            case LIR_iBRHT: case LIR_iBLFT:
            case LIR_bSHR:  case LIR_bSHL:
            case LIR_bOR:   case LIR_bXOR: case LIR_bAND:
            case LIR_iMUL:  case LIR_iSUB: case LIR_iADD: {
                int is_shift = 
                    lh->op == LIR_iBRHT || lh->op == LIR_iBLFT ||
                    lh->op == LIR_bSHR  || lh->op == LIR_bSHL;
                if (
                    !is_shift &&
                    lh->farg->t == LIR_REGISTER && 
                    lh->sarg->t == LIR_REGISTER
                ) break;
                int shared_size = -1;
                if (lh->op == LIR_iMUL) shared_size = lh->sarg->size < 4 ? 4 : lh->sarg->size; 
                lir_subject_t* a_entry = x86_64_macho_nasm_create_tmp(RAX, lh->sarg, smt, shared_size);
                lir_subject_t* a_exit  = x86_64_macho_nasm_create_tmp(RAX, lh->farg, smt, shared_size);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a_entry, lh->sarg, NULL), lh);

                if (is_shift) {
                    lir_subject_t* b_entry = x86_64_macho_nasm_create_tmp(RCX, lh->targ, smt, 1);
                    _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, b_entry, lh->targ, NULL), lh);
                    lh->targ = b_entry;
                }

                _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, lh->farg, a_exit, NULL), lh);
                lh->farg = a_exit;
                lh->sarg = a_entry;
                break;
            }
            case LIR_EXITOP:
            case LIR_FEND:
            case LIR_FRET: {
                if (lh->farg && lh->op != LIR_FEND) {
                    lir_subject_t* a = x86_64_macho_nasm_create_tmp(lh->op == LIR_FRET ? RAX : RDI, lh->farg, smt, -1);
                    _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->farg, NULL), lh);
                    lh->farg = a;
                }

                if (
                    (lh->op == LIR_FRET || lh->op == LIR_FEND) &&
                    fi->flags.abi && !fi->flags.entry &&
                    !set_has(&ctx->callee_restored_blocks, bb)
                ) {
                    lir_registers_t saved[] = { RBX, R12, R13, R14, R15 };
                    if (!set_add(&ctx->callee_restored_blocks, bb)) return CFG_DFS_STOP;
                    for (int i = (int)(sizeof(saved) / sizeof(saved[0])) - 1; i >= 0; i--) {
                        _insert_instruction_before(bb, LIR_create_block(LIR_POP, LIR_SUBJ_REG(saved[i], CONF_get_full_bytness()), NULL, NULL), lh);
                    }
                }

                break;
            }
            case LIR_iDIV:
            case LIR_iMOD: {
                lir_subject_t* a_entry = x86_64_macho_nasm_create_tmp(RAX, lh->sarg, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a_entry, lh->sarg, NULL), lh);

                lir_subject_t* oldres = lh->farg;
                lh->sarg = a_entry;

                lir_subject_t* b = x86_64_macho_nasm_create_tmp(RCX, lh->targ, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, b, lh->targ, NULL), lh);
                lh->targ = b;

                lir_subject_t* mod = x86_64_macho_nasm_create_tmp(RDX, lh->farg, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_PUSH, mod, NULL, NULL), lh);
                _insert_instruction_before(bb, LIR_create_block(LIR_CQO, NULL, NULL, NULL), lh);
                if (lh->op != LIR_iMOD) {
                    lh->farg = x86_64_macho_nasm_create_tmp(RAX, lh->farg, smt, -1);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, oldres, lh->farg, NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_POP, mod, NULL, NULL), lh);
                }
                else {
                    lh->farg = LIR_SUBJ_REG(RDX, lh->farg->size);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, oldres, LIR_SUBJ_REG(RAX, oldres->size), NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_POP, mod, NULL, NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, LIR_SUBJ_REG(RAX, lh->farg->size), lh->farg, NULL), lh);
                }

                break;
            }
            CONDITIONAL_CASE(LIR_CMP, lh->farg->t != LIR_VARIABLE,
                lir_subject_t* a = x86_64_macho_nasm_create_tmp(RAX, lh->farg, smt, -1);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->farg, NULL), lh);
                lh->farg = a;
                break;
            )
            case LIR_iLWR: case LIR_iLRE: case LIR_iLRG: case LIR_iLGE:
            case LIR_iCMP: case LIR_iNMP: {
                lir_subject_t* a   = x86_64_macho_nasm_create_tmp(RAX, lh->sarg, smt, -1);
                lir_subject_t* b   = lh->targ;
                lir_subject_t* res = LIR_SUBJ_REG(AL, 1);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->sarg, NULL), lh);
                _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, lh->farg, res, NULL), lh);

                switch (lh->op) {
                    case LIR_iCMP: _insert_instruction_after(bb, LIR_create_block(LIR_SETE, res, NULL, NULL), lh); break;
                    case LIR_iNMP: _insert_instruction_after(bb, LIR_create_block(LIR_STNE, res, NULL, NULL), lh); break;
                    default: {
                        if (x86_64_macho_nasm_is_sign_type(lh->sarg, smt) && x86_64_macho_nasm_is_sign_type(lh->targ, smt)) {
                            switch (lh->op) {
                                case LIR_iLWR: _insert_instruction_after(bb, LIR_create_block(LIR_SETL, res, NULL, NULL), lh); break;
                                case LIR_iLRE: _insert_instruction_after(bb, LIR_create_block(LIR_STLE, res, NULL, NULL), lh); break;
                                case LIR_iLRG: _insert_instruction_after(bb, LIR_create_block(LIR_SETG, res, NULL, NULL), lh); break;
                                case LIR_iLGE: _insert_instruction_after(bb, LIR_create_block(LIR_STGE, res, NULL, NULL), lh); break;
                                default: break;
                            }
                        }
                        else {
                            switch (lh->op) {
                                case LIR_iLWR: _insert_instruction_after(bb, LIR_create_block(LIR_SETB, res, NULL, NULL), lh); break;
                                case LIR_iLRE: _insert_instruction_after(bb, LIR_create_block(LIR_STBE, res, NULL, NULL), lh); break;
                                case LIR_iLRG: _insert_instruction_after(bb, LIR_create_block(LIR_SETA, res, NULL, NULL), lh); break;
                                case LIR_iLGE: _insert_instruction_after(bb, LIR_create_block(LIR_STAE, res, NULL, NULL), lh); break;
                                default: break;
                            }
                        }
                    }
                }

                lh->op = LIR_CMP;
                lh->farg = a;
                lh->sarg = b;
                break;
            }
            case LIR_TF64: case LIR_TF32: 
            case LIR_TI64: case LIR_TI32: case LIR_TI16: case LIR_TI8:
            case LIR_TU64: case LIR_TU32: case LIR_TU16: case LIR_TU8: {
                lh->op = x86_64_macho_nasm_get_proper_mov(lh->farg, lh->sarg, smt, LIR_iMOV);
                break;
            }
            default: break;
        }
    }

    return CFG_DFS_CONTINUE;
}

int x86_64_macho_nasm_instruction_selection(cfg_ctx_t* cctx, sym_table_t* smt) {
    lir_translate_ctx_t ctx = { 0 };
    queue_t dirty_regs;
    queue_init(&dirty_regs);
    set_init(&ctx.callee_restored_blocks, SET_NO_CMP);

    foreach (cfg_func_t* fb, &cctx->funcs) {
        if (!fb->used) continue;
        
        func_info_t fi;
        if (!FNTB_get_info_id(fb->f_id, &fi, &smt->f)) continue;

        cfg_block_t* hb = list_get_head(&fb->blocks);
        if (hb) {
            if (fi.flags.abi && !fi.flags.entry) {
                lir_registers_t saved[] = { RBX, R12, R13, R14, R15 };
                if (
                    hb->lmap.entry &&
                    (hb->lmap.entry->op == LIR_STRT || hb->lmap.entry->op == LIR_FDCL)
                ) {
                    for (int i = (int)(sizeof(saved) / sizeof(saved[0])) - 1; i >= 0; i--) {
                        _insert_instruction_after(hb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(saved[i], CONF_get_full_bytness()), NULL, NULL), hb->lmap.entry);
                    }
                }
                else if (hb->lmap.entry) {
                    for (int i = 0; i < (int)(sizeof(saved) / sizeof(saved[0])); i++) {
                        _insert_instruction_before(hb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(saved[i], CONF_get_full_bytness()), NULL, NULL), hb->lmap.entry);
                    }
                }
            }

            if (!CFG_DFS_WALK(list_get_head(&fb->blocks), _instruction_selection_block, fb, &fi, &dirty_regs, &ctx, smt)) {
                queue_free(&dirty_regs);
                set_free(&ctx.callee_restored_blocks);
                return 0;
            }
        }
    }

    queue_free(&dirty_regs);
    set_free(&ctx.callee_restored_blocks);
    return 1;
}
