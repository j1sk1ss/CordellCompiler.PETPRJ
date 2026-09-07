#include <lir/copyprop.h>

static int _mark_used_variable_id(symbol_id_t id, sym_table_t* smt) {
    variable_info_t vi;
    while (
        id != NO_SYMBOL_ID &&
        VRTB_get_info_id(id, &vi, &smt->v)
    ) {
        VRTB_set_used(vi.v_id, &smt->v);
        id = vi.p_id;
    }

    return 1;
}

static int _mark_used_variable(lir_subject_t* s, sym_table_t* smt) {
    if (!s) return 0;
    switch (s->t) {
        case LIR_VARIABLE:
        case LIR_GLVARIABLE:
        case LIR_STVARIABLE: _mark_used_variable_id(s->storage.var.v_id, smt); break;
        case LIR_ARGLIST: {
            foreach (lir_subject_t* arg, &s->storage.list.h) {
                _mark_used_variable(arg, smt);
            }
            break;
        }
        default: break;
    }

    return 1;
}

int LIR_clear_global_variables(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        if (!fb->used) continue;
        foreach (cfg_block_t* bb, &fb->blocks) {
            iterate_lir_instructions (bb) {
                if (lh->unused || lh->op == LIR_OEXT) continue;
                iterate_lir_args (lir_subject_t* s, lh, 0) {
                    _mark_used_variable(s, smt);
                }
            }
        }
    }

    return 1;
}
