#include <lir/selector/i386_gnu_nasm.h>
// TODO: Complete AVX support

/* Insert block before 'pos' block with the block entry update.
Params:
    - `bb` - Source block.
    - `b` - New block for an insertion process.
    - `pos` - Position for an insert. */
static inline void _insert_instruction_before(cfg_block_t* bb, lir_block_t* b, lir_block_t* pos) {
    if (!b) return;
    if (bb->lmap.entry == pos) bb->lmap.entry = b;
    LIR_insert_block_before(b, pos);
}

/* Insert block after 'pos' block with the block exit update.
Params:
    - `bb` - Source block.
    - `b` - New block for an insertion process.
    - `pos` - Position for an insert. */
static inline void _insert_instruction_after(cfg_block_t* bb, lir_block_t* b, lir_block_t* pos) {
    if (!b) return;
    if (bb->lmap.exit == pos) bb->lmap.exit = b;
    LIR_insert_block_after(b, pos);
}

/* Count fixed function arguments.
Variadic marker arguments are not counted because they do not occupy a
regular named-argument slot.
Params:
    - `f_id` - Function id.
    - `smt` - Symtable.

Returns count of presented non-variadic arguments. */
static int _count_presented_args(symbol_id_t f_id, sym_table_t* smt) {
    func_info_t fi;
    if (!FNTB_get_info_id(f_id, &fi, &smt->f)) return 0;
    int res = 0;
    fn_iterate_args (&fi) {
        if (arg->t && arg->t->t_type != VAR_ARGUMENTS_TOKEN) res++;
    }

    return res;
}

typedef struct {
    int   clean_stack;
    set_t callee_restored_blocks;
} lir_translate_ctx_t;

static cfg_dfs_action_t _instruction_selection_block(
    cfg_block_t* bb, long pred, cfg_func_t* fb, func_info_t* fi, queue_t* dirty_regs, lir_translate_ctx_t* ctx, sym_table_t* smt
) {
    (void)pred;
    iterate_lir_instructions (bb) {
        switch (lh->op) {
            case LIR_STSARG: {
                int sys_regs[] = { EAX, EBX, ECX, EDX, ESI, EDI, EBP };
                if (lh->sarg->storage.cnst.value >= (long)(sizeof(sys_regs) / sizeof(EAX))) break;
                if (sys_regs[lh->sarg->storage.cnst.value] != EAX) {
                    _insert_instruction_before(
                        bb, LIR_create_block(LIR_PUSH, LIR_SUBJ_REG(sys_regs[lh->sarg->storage.cnst.value], CONF_get_full_bytness()), NULL, NULL), 
                        lh
                    );
                    queue_push(dirty_regs, (void*)((long)sys_regs[lh->sarg->storage.cnst.value]));
                }

                lir_subject_t* nfarg = i386_gnu_nasm_create_tmp(sys_regs[lh->sarg->storage.cnst.value], lh->farg, smt, CONF_get_full_bytness());
                LIR_unload_subject(lh->sarg);
                lh->op   = LIR_aMOV;
                lh->sarg = lh->farg;
                lh->farg = nfarg;
                break;
            }
            case LIR_REF_ARGS: {
                lh->op   = LIR_REF;
                lh->sarg = LIR_SUBJ_OFF(EBP, (!fi->flags.naked + _count_presented_args(fb->f_id, smt) + 1) * -4, CONF_get_full_bytness());
                break;
            }
            case LIR_ECLL:
            case LIR_FCLL: {
                if (ctx->clean_stack) {
                    _insert_instruction_after(bb, LIR_create_block(LIR_iADD, LIR_SUBJ_REG(ESP, CONF_get_full_bytness()), LIR_SUBJ_REG(ESP, CONF_get_full_bytness()), LIR_SUBJ_CONST(ctx->clean_stack)), lh);
                    ctx->clean_stack = 0;
                }
                __attribute__ ((fallthrough));
            }
            case LIR_SYSC: {
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
                        src = LIR_SUBJ_OFF(EBP, -4, CONF_get_full_bytness());
                        lh->op = LIR_iMOV; 
                        break;
                    }
                    default: {
                        src = LIR_SUBJ_OFF(EBP, -8, CONF_get_full_bytness());
                        lh->op = LIR_REF;  
                        break;
                    }
                }

                LIR_unload_subject(lh->sarg);
                lh->sarg = src;
                break;
            }
            case LIR_STFARG: {
                lh->op           = LIR_PUSH;
                lh->farg->size   = 4;
                ctx->clean_stack += 4;
                break;
            }
            case LIR_LOADFARG: {
                lir_subject_t* nfarg = LIR_SUBJ_OFF(EBP, (lh->sarg->storage.cnst.value + !fi->flags.naked + 1) * -4, lh->farg->size);
                LIR_unload_subject(lh->sarg);
                lh->op   = LIR_phiMOV;
                lh->sarg = nfarg;
                break;
            }
            case LIR_LOADFRET: {
                lh->op   = LIR_iMOV;
                lh->sarg = i386_gnu_nasm_create_tmp(EAX, lh->farg, smt, -1);
                break;
            }
            case LIR_NOT: {
                lir_subject_t* a   = i386_gnu_nasm_create_tmp(EAX, lh->sarg, smt, -1);
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
                if (lh->op == LIR_iMUL) shared_size = lh->sarg->size < 2 ? 2 : lh->sarg->size; 
                lir_subject_t* a_entry = i386_gnu_nasm_create_tmp(EAX, lh->sarg, smt, shared_size);
                lir_subject_t* a_exit  = i386_gnu_nasm_create_tmp(EAX, lh->farg, smt, shared_size);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a_entry, lh->sarg, NULL), lh);
                
                if (is_shift) {
                    lir_subject_t* b_entry = i386_gnu_nasm_create_tmp(ECX, lh->targ, smt, 1);
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
                    lir_subject_t* a = i386_gnu_nasm_create_tmp(lh->op == LIR_FRET ? EAX : EBX, lh->farg, smt, -1);
                    _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->farg, NULL), lh);
                    lh->farg = a;
                }

                if (
                    (lh->op == LIR_FRET || lh->op == LIR_FEND) &&
                    fi->flags.abi && !fi->flags.entry &&
                    !set_has(&ctx->callee_restored_blocks, bb)
                ) {
                    lir_registers_t saved[] = { EBX, ESI, EDI };
                    if (!set_add(&ctx->callee_restored_blocks, bb)) return CFG_DFS_STOP;
                    for (int i = (int)(sizeof(saved) / sizeof(saved[0])) - 1; i >= 0; i--) {
                        _insert_instruction_before(bb, LIR_create_block(LIR_POP, LIR_SUBJ_REG(saved[i], CONF_get_full_bytness()), NULL, NULL), lh);
                    }
                }

                break;
            }
            case LIR_iDIV:
            case LIR_iMOD: {
                lir_subject_t* a_entry = i386_gnu_nasm_create_tmp(EAX, lh->sarg, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a_entry, lh->sarg, NULL), lh);

                lir_subject_t* oldres = lh->farg;
                lh->sarg = a_entry;

                lir_subject_t* b = i386_gnu_nasm_create_tmp(ECX, lh->targ, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, b, lh->targ, NULL), lh);
                lh->targ = b;

                lir_subject_t* mod = i386_gnu_nasm_create_tmp(EDX, lh->farg, smt, CONF_get_full_bytness());
                _insert_instruction_before(bb, LIR_create_block(LIR_PUSH, mod, NULL, NULL), lh);
                _insert_instruction_before(bb, LIR_create_block(LIR_CDQ, NULL, NULL, NULL), lh);
                if (lh->op != LIR_iMOD) {
                    lh->farg = i386_gnu_nasm_create_tmp(EAX, lh->farg, smt, -1);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, oldres, lh->farg, NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_POP, mod, NULL, NULL), lh);
                }
                else {
                    lh->farg = LIR_SUBJ_REG(EDX, lh->farg->size);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, oldres, LIR_SUBJ_REG(EAX, oldres->size), NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_POP, mod, NULL, NULL), lh);
                    _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, LIR_SUBJ_REG(EAX, lh->farg->size), lh->farg, NULL), lh);
                }

                break;
            }
            CONDITIONAL_CASE(LIR_CMP, lh->farg->t != LIR_VARIABLE,
                lir_subject_t* a = i386_gnu_nasm_create_tmp(EAX, lh->farg, smt, -1);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->farg, NULL), lh);
                lh->farg = a;
                break;
            )
            case LIR_iLWR: case LIR_iLRE: case LIR_iLRG: case LIR_iLGE:
            case LIR_iCMP: case LIR_iNMP: {
                lir_subject_t* a   = i386_gnu_nasm_create_tmp(EAX, lh->sarg, smt, -1);
                lir_subject_t* b   = lh->targ;
                lir_subject_t* res = LIR_SUBJ_REG(AL, 1);
                _insert_instruction_before(bb, LIR_create_block(LIR_iMOV, a, lh->sarg, NULL), lh);
                _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, lh->farg, res, NULL), lh);

                switch (lh->op) {
                    case LIR_iCMP: _insert_instruction_after(bb, LIR_create_block(LIR_SETE, res, NULL, NULL), lh); break;
                    case LIR_iNMP: _insert_instruction_after(bb, LIR_create_block(LIR_STNE, res, NULL, NULL), lh); break;
                    default: {
                        if (i386_gnu_nasm_is_sign_type(lh->sarg, smt) && i386_gnu_nasm_is_sign_type(lh->targ, smt)) {
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

                lh->op   = LIR_CMP;
                lh->farg = a;
                lh->sarg = b;
                break;
            }
            case LIR_TF64: case LIR_TF32: 
            case LIR_TI64: case LIR_TI32: case LIR_TI16: case LIR_TI8:
            case LIR_TU64: case LIR_TU32: case LIR_TU16: case LIR_TU8: {
                lir_subject_t* dst = lh->farg;
                lir_subject_t* src = lh->sarg;
                lh->op = i386_gnu_nasm_get_proper_mov(dst, src, smt, LIR_iMOV);

                switch (lh->op) {
                    case LIR_CVTTSS2SI:
                    case LIR_CVTTSD2SI: {
                        lir_subject_t* tmp = i386_gnu_nasm_create_tmp(EAX, dst, smt, CONF_get_full_bytness());
                        _insert_instruction_after(bb, LIR_create_block(LIR_iMOV, dst, tmp, NULL), lh);
                        lh->farg = tmp;
                        break;
                    }
                    case LIR_CVTSI2SS:
                    case LIR_CVTSI2SD: {
                        if (src->size < 4) {
                            lir_subject_t* tmp = i386_gnu_nasm_create_tmp(EAX, src, smt, CONF_get_full_bytness());
                            lir_operation_t op = i386_gnu_nasm_get_proper_mov(tmp, src, smt, LIR_iMOV);
                            _insert_instruction_before(bb, LIR_create_block(op, tmp, src, NULL), lh);
                            lh->sarg = tmp;
                        }

                        lir_subject_t* tmp = i386_gnu_nasm_create_tmp(XMM0, dst, smt, dst->size);
                        _insert_instruction_after(bb, LIR_create_block(LIR_fMOV, dst, tmp, NULL), lh);
                        lh->farg = tmp;
                        break;
                    }
                    case LIR_CVTSS2SD:
                    case LIR_CVTSD2SS: {
                        lir_subject_t* tmp = i386_gnu_nasm_create_tmp(XMM0, dst, smt, dst->size);
                        _insert_instruction_after(bb, LIR_create_block(LIR_fMOV, dst, tmp, NULL), lh);
                        lh->farg = tmp;
                        break;
                    }
                    default: break;
                }

                break;
            }
            default: break;
        }
    }

    return CFG_DFS_CONTINUE;
}

int i386_gnu_nasm_instruction_selection(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        if (!fb->used) continue;
        
        func_info_t fi;
        if (!FNTB_get_info_id(fb->f_id, &fi, &smt->f)) continue;

        lir_translate_ctx_t ctx = { 0 };
        queue_t dirty_regs;
        queue_init(&dirty_regs);
        set_init(&ctx.callee_restored_blocks, SET_NO_CMP);

        cfg_block_t* hb = list_get_head(&fb->blocks);
        if (hb) {
            if (fi.flags.abi && !fi.flags.entry) {
                lir_registers_t saved[] = { EBX, ESI, EDI };
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

        queue_free(&dirty_regs);
        set_free(&ctx.callee_restored_blocks);
    }

    return 1;
}
