#include <lir/constfold.h>

/* Get the variable's ID and get the definition of the variable.
Params:
    - `s` - LIR subject for constant fold.
    - `smt` - Symtable.

Returns 1 if folding completed, otherwise 0. */
static int _apply_constfold_on_subject(lir_subject_t* s, sym_table_t* smt) {
    if (s->t != LIR_VARIABLE) return 0;
    if (ALLIAS_get_owners(s->storage.var.v_id, NULL, &smt->m)) return 0;
    variable_info_t vi;
    if (!VRTB_get_info_id(s->storage.var.v_id, &vi, &smt->v)) return 0;
    if (vi.vdi.defined == DEFINED_VARIABLE) {
        s->storage.cnst.value = vi.vdi.definition;
        s->t = LIR_CONSTVAL;
        return 1;
    }

    return 0;
}

int LIR_apply_sparse_const_propagation(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* bb, &fb->blocks) {
            iterate_lir_instructions (bb) {
                iterate_lir_args (lir_subject_t* arg, lh, 0) {
                    switch (arg->t) {
                        case LIR_ARGLIST: {
                            foreach (lir_subject_t* s, &arg->storage.list.h) {
                                _apply_constfold_on_subject(s, smt);
                            }
                            
                            break;
                        }
                        default: _apply_constfold_on_subject(arg, smt); break;
                    }
                }
            }
        }
    }

    return 1;
}
