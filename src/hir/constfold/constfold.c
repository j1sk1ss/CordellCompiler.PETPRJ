#include <hir/constfold.h>

typedef struct {
    symbol_id_t        overdefined;
    long               value;
    hir_subject_type_t t;
} const_t;

/* Parse input DAG node as a constant.
Note: Will work only with a NUMBER or CONSTANT DAG node.
Params:
    - `nd` - End DAG node.
             Note: 'End' means that this is the last DAG node 
                   in the DAG.
    - `cnst` - Const placeholder.
    - `smt` - Symtable.

Returns 1 if parsing succeeded, otherwise 0. */
static int _parse_const(dag_node_t* nd, const_t* cnst, sym_table_t* smt) {
    if (!nd) return 0;
    cnst->overdefined = NO_SYMBOL_ID;
    switch (nd->src->t) {
        case HIR_I64NUMBER: cnst->t = HIR_I64CONSTVAL; goto _parse_number_complete;
        case HIR_U64NUMBER: cnst->t = HIR_U64CONSTVAL; goto _parse_number_complete;
        case HIR_I32NUMBER: cnst->t = HIR_I32CONSTVAL; goto _parse_number_complete;
        case HIR_U32NUMBER: cnst->t = HIR_U32CONSTVAL; goto _parse_number_complete;
        case HIR_I16NUMBER: cnst->t = HIR_I16CONSTVAL; goto _parse_number_complete;
        case HIR_U16NUMBER: cnst->t = HIR_U16CONSTVAL; goto _parse_number_complete;
        case HIR_I8NUMBER:  cnst->t = HIR_I8CONSTVAL;  goto _parse_number_complete;
        case HIR_U8NUMBER:  cnst->t = HIR_U8CONSTVAL;  goto _parse_number_complete;
        case HIR_NUMBER:    cnst->t = HIR_I8CONSTVAL;  goto _parse_number_complete;
        case HIR_F64NUMBER:
        case HIR_F32NUMBER:
        {
_parse_number_complete: {}
            cnst->value = nd->src->storage.num.value->to_llong(nd->src->storage.num.value);
            return 1;
        }
        case HIR_U8CONSTVAL:  case HIR_I8CONSTVAL:
        case HIR_U16CONSTVAL: case HIR_I16CONSTVAL:
        case HIR_U32CONSTVAL: case HIR_I32CONSTVAL:
        case HIR_U64CONSTVAL: case HIR_I64CONSTVAL: {
            cnst->t = nd->src->t;
            cnst->value = nd->src->storage.cnst.value;
            return 1;
        }
        default: {
            variable_info_t vi;
            if (HIR_is_vartype(nd->src->t) && VRTB_get_info_id(nd->src->storage.var.v_id, &vi, &smt->v) && vi.vdi.defined) {
                cnst->t     = HIR_I64CONSTVAL;
                cnst->value = vi.vdi.definition;
                if (vi.vdi.defined == OVERDEFINED_VARIABLE) {
                    cnst->overdefined = vi.vdi.definition;
                }

                return 1;
            }

            return 0;
        }
    }
}

/* Flatten the arguments of the provided DAG node.
Params:
    - `nd` - DAG node.
    - `args` - Output flatten array.

Returns 1 if arguments are flattened, otherwise 0. */
static int _const_args(dag_node_t* nd, dag_node_t** args, int args_size) {
    int i = 0;
    set_foreach (dag_node_t* arg, &nd->args) {
        if (i + 1 >= args_size) return 0;
        args[i++] = arg;
    }

    return 1;
}

int HIR_sparse_const_propagation(dag_ctx_t* dctx, sym_table_t* smt) {
    int changed = 0;
    do {
        changed = 0;
        map_foreach (dag_node_t* nd, &dctx->dag) {
            switch (nd->op) {
                case HIR_PHI: {
                    if (!nd->src->home || !nd->src->home->targ) break;
                    long first = 1, value = 0, same = 1;
                    set_foreach (int_tuple_t* t, &nd->src->home->targ->storage.set.h) {
                        variable_info_t vi; 
                        if (!VRTB_get_info_id(t->y, &vi, &smt->v) || vi.vdi.defined != DEFINED_VARIABLE) same = 0;
                        else {
                            if (first) value = vi.vdi.definition;
                            else if (!first && value != vi.vdi.definition) same = 0;
                        }
                        
                        if (!same) break;
                        first = 0;
                    }
                    
                    if (same) {
                        if (VRTB_update_definition(nd->src->storage.var.v_id, value, NO_SYMBOL_ID, &smt->v, 1)) changed = 1;
                    }
                    else {
                        if (VRTB_update_definition(nd->src->storage.var.v_id, FIELD_NO_CHANGE, nd->src->storage.var.v_id, &smt->v, 0)) changed = 1;
                    }

                    continue;
                }
                case HIR_GDREF: {
                    if (!nd->src->home || !nd->src->home->farg) break;
                    set_t slaves;
                    
                    if (
                        nd->src->home->sarg && 
                        ALLIAS_get_slaves(nd->src->home->sarg->storage.var.v_id, &slaves, &smt->m)
                    ) {
                        set_foreach (symbol_id_t slave, &slaves) {
                            variable_info_t vi; 
                            if (VRTB_get_info_id(slave, &vi, &smt->v) && vi.vdi.defined == DEFINED_VARIABLE) {
                                if (VRTB_update_definition(nd->src->storage.var.v_id, vi.vdi.definition, NO_SYMBOL_ID, &smt->v, 0)) changed = 1;
                                break;
                            } 
                        }
                    }

                    set_free(&slaves);
                    continue;
                }
                default: break;
            }

            dag_node_t* args[4] = { NULL };
            if (!_const_args(nd, args, 4)) {
                print_error("Argument flattening error!");
                return 0;
            }
            
            if (
                !HIR_is_vartype(nd->src->t)              /* If considered object isn't a variable     */
            ) continue;                                  /* We skip such variables / objects given    */
                                                         /* the necessity of preserving over          */
                                                         /* 'inlining'.                               */
            const_t a, b;
            int a_pres = _parse_const(args[0], &a, smt);
            int b_pres = _parse_const(args[1], &b, smt);
            if (!args[1]) {                              /* Parse constants, and if there is no       */
                                                         /* the second argument, copy value from the  */
                                                         /* first one.                                */
                str_memcpy(&b, &a, sizeof(const_t));
                b_pres = 1;
            }
            
            long long c = 0;
            print_debug("src=%li, a_pres=%i, b_pres=%i", nd->src->storage.var.v_id, a_pres, b_pres);
            switch (nd->op) {
                case HIR_STORE: {
                    if (!a_pres) break;
                    if (VRTB_update_definition(nd->src->storage.var.v_id, a.value, a.overdefined, &smt->v, 1)) changed = 1;
                    print_debug("Store operation folded into val=%ld", nd->op, a.value);
                    break;
                }
                case HIR_TF64:
                case HIR_TF32: {
                    if (!a_pres) break;
                    if (!HIR_is_float(a.t)) c = str_dob2bits((double)a.value);
                    else c = a.value;
                    if (VRTB_update_definition(nd->src->storage.var.v_id, c, a.overdefined, &smt->v, 1)) changed = 1;
                    print_debug("Convert op=%i folded to the val=%ld", nd->op, c);
                    break;
                }
                case HIR_TI64:
                case HIR_TI32:
                case HIR_TI16:
                case HIR_TI8:
                case HIR_TPTR:
                case HIR_TU64:
                case HIR_TU32:
                case HIR_TU16:
                case HIR_TU8: {
                    if (!a_pres) break;

                    c = a.value;
                    if (HIR_is_float(a.t)) {
                        c = (long long)str_bits2dob(c);
                    }

                    switch (nd->op) {
                        case HIR_TI32: c = (int)c;            break;
                        case HIR_TI16: c = (short)c;          break;
                        case HIR_TI8:  c = (char)c;           break;
                        case HIR_TPTR:
                        case HIR_TU64: c = (unsigned long)c;  break;
                        case HIR_TU32: c = (unsigned int)c;   break;
                        case HIR_TU16: c = (unsigned short)c; break;
                        case HIR_TU8:  c = (unsigned char)c;  break;
                        default: break;
                    }

                    if (VRTB_update_definition(nd->src->storage.var.v_id, c, a.overdefined, &smt->v, 1)) changed = 1;
                    print_debug("Convert op=%i folded to the val=%ld", nd->op, c);
                    break;
                }
                case HIR_NEG: {
                    if (!a_pres) break;
                    if (VRTB_update_definition(nd->src->storage.var.v_id, ~a.value, a.overdefined, &smt->v, 1)) changed = 1;
                    print_debug("Not op=%i folded to the val=%ld", nd->op, ~a.value);
                    break;
                }
                case HIR_NOT: {
                    if (!a_pres) break;
                    if (VRTB_update_definition(nd->src->storage.var.v_id, !a.value, a.overdefined, &smt->v, 1)) changed = 1;
                    print_debug("Not op=%i folded to the val=%ld", nd->op, !a.value);
                    break;
                }
                case HIR_iADD:  c = a.value + b.value;  goto _binary_operation_fold;
                case HIR_iSUB:  c = a.value - b.value;  goto _binary_operation_fold;
                case HIR_iMUL:  c = a.value * b.value;  goto _binary_operation_fold;
                case HIR_iLRG:  c = a.value > b.value;  goto _binary_operation_fold;
                case HIR_iLGE:  c = a.value >= b.value; goto _binary_operation_fold;
                case HIR_iLWR:  c = a.value < b.value;  goto _binary_operation_fold;
                case HIR_iLRE:  c = a.value <= b.value; goto _binary_operation_fold;
                case HIR_iCMP:  c = a.value == b.value; goto _binary_operation_fold;
                case HIR_iNMP:  c = a.value != b.value; goto _binary_operation_fold;
                case HIR_iAND:  c = a.value && b.value; goto _binary_operation_fold;
                case HIR_iOR:   c = a.value || b.value; goto _binary_operation_fold;
                case HIR_iBLFT: c = a.value << b.value; goto _binary_operation_fold;
                case HIR_iBRHT: c = a.value >> b.value; goto _binary_operation_fold;
                case HIR_bAND:  c = a.value & b.value;  goto _binary_operation_fold;
                case HIR_bOR:   c = a.value | b.value;  goto _binary_operation_fold;
                case HIR_iMOD:  c = a.value % (!b.value ? 1 : b.value); goto _binary_operation_fold;
                case HIR_iDIV:  c = a.value / (!b.value ? 1 : b.value); goto _binary_operation_fold;
                case HIR_bXOR:  c = a.value ^ b.value; {
_binary_operation_fold: {}
                    if (!a_pres || !b_pres || a.overdefined != NO_SYMBOL_ID || b.overdefined != NO_SYMBOL_ID) break;
                    if ((nd->op == HIR_iDIV || nd->op == HIR_iMOD) && !b.value) break;
                    if (VRTB_update_definition(nd->src->storage.var.v_id, c, NO_SYMBOL_ID, &smt->v, 1)) changed = 1;
                    print_debug("%ld op=%i %ld folded into val=%i", a.value, nd->op, b.value, c);
                    break;
                }

                default: break;
            }
        }
    } while (changed);
    return 1;
}
