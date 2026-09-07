#include <lir/lirgens/lirgens.h>

static lir_subject_t* _convert_hs_to_ls(hir_subject_t* subj) {
    if (!subj) return NULL;
    switch (subj->t) {
        case HIR_F64NUMBER: case HIR_F32NUMBER: return LIR_SUBJ_NUMBER(subj->storage.num.value, 1, CONF_get_full_bytness());
        case HIR_I64NUMBER: case HIR_U64NUMBER: case HIR_NUMBER: return LIR_SUBJ_NUMBER(subj->storage.num.value, 0, CONF_get_full_bytness());
        case HIR_I32NUMBER: case HIR_U32NUMBER: return LIR_SUBJ_NUMBER(subj->storage.num.value, 0, CONF_get_half_bytness());
        case HIR_I16NUMBER: case HIR_U16NUMBER: return LIR_SUBJ_NUMBER(subj->storage.num.value, 0, CONF_get_quart_bytness());
        case HIR_I8NUMBER:  case HIR_U8NUMBER:  return LIR_SUBJ_NUMBER(subj->storage.num.value, 0, CONF_get_eight_bytness());

        case HIR_U8CONSTVAL:  case HIR_I8CONSTVAL:
        case HIR_U16CONSTVAL: case HIR_I16CONSTVAL:
        case HIR_U32CONSTVAL: case HIR_I32CONSTVAL:
        case HIR_U64CONSTVAL: case HIR_I64CONSTVAL:
                         return LIR_SUBJ_CONST(subj->storage.cnst.value);
        case HIR_RAWASM: return LIR_SUBJ_RAWASM(subj->storage.str.s_id);
        case HIR_STRING: return LIR_SUBJ_STRING(subj->storage.str.s_id);
        case HIR_FNAME:  return LIR_SUBJ_ADDRFUNC(subj);
        case HIR_FPOS:   return LIR_SUBJ_LOCATION(&subj->storage.pos);
        
        case HIR_TMPVARF64: case HIR_TMPVARF32:
        case HIR_STKVARF64: case HIR_STKVARF32: 
        case HIR_GLBVARF64: case HIR_GLBVARF32:
        case HIR_TMPVARARR: case HIR_TMPVARI64: case HIR_TMPVARU64:
        case HIR_TMPVARU32: case HIR_TMPVARI32: case HIR_TMPVARU16: case HIR_TMPVARI16: 
        case HIR_TMPVARU8:  case HIR_TMPVARI8:  case HIR_TMPVARI0:
        case HIR_STKVARARR: 
        case HIR_STKVARU64: case HIR_STKVARI64: 
        case HIR_STKVARU32: case HIR_STKVARI32: case HIR_STKVARU16: case HIR_STKVARI16: 
        case HIR_STKVARU8:  case HIR_STKVARI8:  case HIR_STKVARI0:
        case HIR_GLBVARARR: 
        case HIR_GLBVARU64: case HIR_GLBVARI64: 
        case HIR_GLBVARU32: case HIR_GLBVARI32: case HIR_GLBVARU16: case HIR_GLBVARI16: 
        case HIR_GLBVARU8:  case HIR_GLBVARI8:  case HIR_GLBVARI0: {
            lir_subject_t* v = LIR_SUBJ_VAR(subj->storage.var.v_id, subj->ptr > 0 ? CONF_get_full_bytness() : HIR_get_type_size(subj->t));
            v->dsize = subj->ptr - 1 > 0 ? v->size : HIR_get_type_size(subj->t);
            return v;
        }
        default: return NULL;
    }
}

static int _translate_params_list(lir_operation_t op, lir_ctx_t* ctx, list_t* hir_args, list_t* lir_args) {
    int argnum = 0, arg_count = list_size(hir_args) - 1;
    list_iter_t it;
    list_iter_tinit(hir_args, &it);
    hir_subject_t* hir_arg;
    while ((hir_arg = list_iter_prev(&it))) {
        lir_subject_t* lir_arg = _convert_hs_to_ls(hir_arg);
        list_add(lir_args, lir_arg);
        LIR_BLOCK2(ctx, op, lir_arg, LIR_SUBJ_CONST(arg_count - argnum++));
    }

    return 1;
}

static int _convert_hir_to_lir(sstack_t* params, hir_block_t* h, lir_ctx_t* ctx, sym_table_t* smt) {
    switch (h->op) {
        case HIR_REF_ARGS:     return LIR_BLOCK1(ctx, LIR_REF_ARGS, _convert_hs_to_ls(h->farg));
        case HIR_PHI_PREAMBLE: return LIR_BLOCK2(ctx, LIR_phiMOV, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_STORE:        return LIR_BLOCK2(ctx, LIR_iMOV, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_STARGLD:      return LIR_BLOCK2(ctx, LIR_STARGLD, _convert_hs_to_ls(h->farg), LIR_SUBJ_CONST(h->sarg->storage.cnst.value)); 
        case HIR_STRT:         return LIR_BLOCK1(ctx, LIR_STRT, LIR_SUBJ_FUNCNAME(h->farg));
        case HIR_OEXT:         return LIR_BLOCK1(ctx, LIR_OEXT, LIR_SUBJ_CONST(h->farg->storage.cnst.value));
        case HIR_FEXT: {
            func_info_t e_fi;
            if (FNTB_get_info_id(h->farg->storage.cnst.value, &e_fi, &smt->f) && !e_fi.flags.used) return 0;
            return LIR_BLOCK1(ctx, LIR_FEXT, LIR_SUBJ_CONST(h->farg->storage.cnst.value));
        }
        case HIR_EXITOP:       return LIR_BLOCK1(ctx, LIR_EXITOP, _convert_hs_to_ls(h->farg));
        case HIR_FDCL:         return LIR_BLOCK1(ctx, LIR_FDCL, LIR_SUBJ_FUNCNAME(h->farg));
        case HIR_FRET:         return LIR_BLOCK1(ctx, LIR_FRET, _convert_hs_to_ls(h->farg));
        case HIR_FARGLD:       return LIR_BLOCK3(ctx, LIR_LOADFARG, _convert_hs_to_ls(h->farg), LIR_SUBJ_CONST(h->sarg->storage.cnst.value), LIR_SUBJ_CONST(h->targ->storage.cnst.value));
        case HIR_UFCLL:       case HIR_FCLL:       case HIR_ECLL: 
        case HIR_STORE_UFCLL: case HIR_STORE_FCLL: case HIR_STORE_ECLL: {
            lir_subject_t* sargs = LIR_SUBJ_LIST();
            _translate_params_list(LIR_STFARG, ctx, &h->targ->storage.list.h, &sargs->storage.list.h);
            LIR_BLOCK3(
                ctx, LIR_FCLL, 
                h->sarg->t == HIR_FNAME ? LIR_SUBJ_FUNCNAME(h->sarg) : _convert_hs_to_ls(h->sarg), 
                NULL, sargs
            );

            if (
                h->op == HIR_STORE_UFCLL || h->op == HIR_STORE_FCLL || h->op == HIR_STORE_ECLL
            ) LIR_BLOCK1(ctx, LIR_LOADFRET, _convert_hs_to_ls(h->farg));
            return 1;
        }
        case HIR_MKSCOPE:  return LIR_BLOCK0(ctx, LIR_MKSCOPE);
        case HIR_ENDSCOPE: return LIR_BLOCK0(ctx, LIR_ENDSCOPE);
        case HIR_SYSC: 
        case HIR_STORE_SYSC: {
            lir_subject_t* sargs = LIR_SUBJ_LIST();
            _translate_params_list(LIR_STSARG, ctx, &h->targ->storage.list.h, &sargs->storage.list.h);
            LIR_BLOCK3(ctx, LIR_SYSC, NULL, NULL, sargs);
            if (h->op == HIR_STORE_SYSC) LIR_BLOCK1(ctx, LIR_LOADFRET, _convert_hs_to_ls(h->farg));
            return 1;
        }
        case HIR_BREAKPOINT: return LIR_BLOCK1(ctx, LIR_BREAKPOINT, h->farg ? LIR_SUBJ_STRING(h->farg->storage.str.s_id) : NULL);
        case HIR_RAW: {
            str_info_t si;
            if (!STTB_get_info_id(h->farg->storage.str.s_id, &si, &smt->s)) return 0;
            int argnum = -1;
            string_t* p = si.value->fchar(si.value, '%');
            if (p) {
                argnum = 0;
                char* head = p->head + 1;
                while (head && *head && str_isdigit(*head)) {
                    argnum = argnum * 10 + (*head - '0');
                    head++;
                }

                destroy_string(p);
            }

            lir_subject_t* subj = NULL;
            if (argnum >= 0) subj = _convert_hs_to_ls(params->data[params->top - argnum].d);
            return LIR_BLOCK2(ctx, LIR_RAW, LIR_SUBJ_RAWASM(h->farg->storage.str.s_id), subj);
        }
        case HIR_STASM: {
            list_iter_t it;
            list_iter_tinit(&h->targ->storage.list.h, &it);
            hir_subject_t* s;
            while ((s = list_iter_prev(&it))) {
                stack_push(params, s);
            }
            
            return 1;
        }
        case HIR_ENDASM: {
            for (int i = 0; i < h->targ->storage.cnst.value; i++) {
                hir_subject_t* s;
                if (stack_pop(params, (void**)&s)) {
                    LIR_BLOCK1(ctx, LIR_VRUSE, _convert_hs_to_ls(s));
                }
            }
            return 1;
        }
        case HIR_STRDECL: return LIR_BLOCK2(ctx, LIR_STRDECL, _convert_hs_to_ls(h->farg), LIR_SUBJ_STRING(h->sarg->storage.str.s_id));
        case HIR_ARRDECL: {
            lir_subject_t* lir_elems = LIR_SUBJ_LIST();
            foreach (hir_subject_t* hir_elem, &h->targ->storage.list.h) {
                list_add(&lir_elems->storage.list.h, _convert_hs_to_ls(hir_elem));
            }

            return LIR_BLOCK3(ctx, LIR_ARRDECL, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), lir_elems);
        }
        case HIR_REF:   return LIR_BLOCK2(ctx, LIR_REF, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));  
        case HIR_GDREF: return LIR_BLOCK2(ctx, LIR_GDREF, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_LDREF: return LIR_BLOCK2(ctx, LIR_LDREF, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_FEND:  return LIR_BLOCK0(ctx, LIR_FEND);
        case HIR_STEND: return LIR_BLOCK0(ctx, LIR_STEND);
        case HIR_TF64:  return LIR_BLOCK2(ctx, LIR_TF64, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TF32:  return LIR_BLOCK2(ctx, LIR_TF32, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TI64:  return LIR_BLOCK2(ctx, LIR_TI64, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TI32:  return LIR_BLOCK2(ctx, LIR_TI32, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TI16:  return LIR_BLOCK2(ctx, LIR_TI16, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TI8:   return LIR_BLOCK2(ctx, LIR_TI8,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_TPTR:
        case HIR_TU64:  return LIR_BLOCK2(ctx, LIR_TU64, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_TU32:  return LIR_BLOCK2(ctx, LIR_TU32, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_TU16:  return LIR_BLOCK2(ctx, LIR_TU16, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_TU8:   return LIR_BLOCK2(ctx, LIR_TU8,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_JMP:   return LIR_BLOCK1(ctx, LIR_JMP, LIR_SUBJ_LABEL(h->farg->id));
        case HIR_MKLB:  return LIR_BLOCK1(ctx, LIR_MKLB, LIR_SUBJ_LABEL(h->farg->id));
        case HIR_NEG:   return LIR_BLOCK2(ctx, LIR_NEG, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg));
        case HIR_NOT:   return LIR_BLOCK2(ctx, LIR_NOT, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg)); 
        case HIR_IFOP2: {
            LIR_BLOCK2(ctx, LIR_CMP, _convert_hs_to_ls(h->farg), LIR_SUBJ_CONST(0));
            LIR_BLOCK1(ctx, LIR_JE, LIR_SUBJ_LABEL(h->targ->id));
            return LIR_BLOCK1(ctx, LIR_JNE, LIR_SUBJ_LABEL(h->sarg->id));
        }
        case HIR_iBLFT:  return LIR_BLOCK3(ctx, LIR_iBLFT, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iBRHT:  return LIR_BLOCK3(ctx, LIR_iBRHT, _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iLWR:   return LIR_BLOCK3(ctx, LIR_iLWR,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iLRE:   return LIR_BLOCK3(ctx, LIR_iLRE,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iLRG:   return LIR_BLOCK3(ctx, LIR_iLRG,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iLGE:   return LIR_BLOCK3(ctx, LIR_iLGE,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iCMP:   return LIR_BLOCK3(ctx, LIR_iCMP,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iNMP:   return LIR_BLOCK3(ctx, LIR_iNMP,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iOR:    return LIR_BLOCK3(ctx, LIR_iOR,   _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iAND:   return LIR_BLOCK3(ctx, LIR_iAND,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_bOR:    return LIR_BLOCK3(ctx, LIR_bOR,   _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_bXOR:   return LIR_BLOCK3(ctx, LIR_bXOR,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_bAND:   return LIR_BLOCK3(ctx, LIR_bAND,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iMOD:   return LIR_BLOCK3(ctx, LIR_iMOD,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iSUB:   return LIR_BLOCK3(ctx, LIR_iSUB,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iDIV:   return LIR_BLOCK3(ctx, LIR_iDIV,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iMUL:   return LIR_BLOCK3(ctx, LIR_iMUL,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_iADD:   return LIR_BLOCK3(ctx, LIR_iADD,  _convert_hs_to_ls(h->farg), _convert_hs_to_ls(h->sarg), _convert_hs_to_ls(h->targ));
        case HIR_VRUSE:  return LIR_BLOCK1(ctx, LIR_VRUSE, _convert_hs_to_ls(h->farg));
        case HIR_SETPOS: return LIR_BLOCK1(ctx, LIR_SETPOS, _convert_hs_to_ls(h->farg));
        default: return 0;
    }
}

static int _iterate_block(sstack_t* params, cfg_block_t* bb, lir_ctx_t* ctx, sym_table_t* smt) { 
    LIR_BLOCK1(ctx, LIR_BB, LIR_SUBJ_CONST(bb->id));
    bb->lmap.entry = ctx->t;

    iterate_hir_instructions (bb) {
        if (hh->unused) continue;
        _convert_hir_to_lir(params, hh, ctx, smt);
    }

    if (!bb->lmap.entry) bb->lmap.entry = ctx->h;
    else bb->lmap.entry = bb->lmap.entry->next;
    bb->lmap.exit = ctx->t;
    return 1;
}

int LIR_destroy_ssa(cfg_ctx_t* cctx) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* bb, &fb->blocks) {
            lir_block_t* lh = LIR_get_next(bb->lmap.entry, bb->lmap.exit, 0);
            while (lh) {
                if (lh->op == LIR_phiMOV) lh->op = LIR_iMOV;
                lh = LIR_get_next(lh, bb->lmap.exit, 1);
            }
        }
    }

    return 1;
}

int LIR_generate_block(cfg_ctx_t* cctx, lir_ctx_t* ctx, sym_table_t* smt) {
    sstack_t params;
    stack_init(&params);

    /* Convert outer blocks to LIR context.
       Note: This blocks go to the highest part of the context. */
    foreach (hir_block_t* o, &cctx->outs.hout) {
        _convert_hir_to_lir(&params, o, ctx, smt);
    }

    for (lir_block_t* lh = ctx->h; lh; lh = lh->next) {
        list_add(&cctx->outs.lout, lh);
        if (lh == ctx->t) break;
    }

    /* Iterate on CFG blocks and convert all alligned HIR blocks
       to LIR blocks. */
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* cb, &fb->blocks) {
            _iterate_block(&params, cb, ctx, smt);
        }

        fb->lmap.entry = ((cfg_block_t*)list_get_head(&fb->blocks))->lmap.entry;
        fb->lmap.exit  = ctx->t;
    }

    stack_free(&params);
    return 1;
}
