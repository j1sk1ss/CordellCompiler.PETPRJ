#include <lir/dfg.h>

/* Perform the high-level scope-based deallocation.
The main idea here is collection and deallocation of all variables out of their scope.
Let's consider the next example:
```cpl
ptr i32 p;
{
    i32 a;
    p = ref a;
: dealloc(a); :
}
dref p = 0; : <= Pointer to 'freed' location :
```

Params:
    - `fb` - Current function CFG part.

Returns 1 on success, otherwise 0. */
static int _scope_pass(cfg_func_t* fb) {
    sstack_t scopes;
    stack_init(&scopes);

    iterate_lir_instructions (fb) {
        switch (lh->op) {
            case LIR_MKSCOPE: {
                list_t* scope_defs = (list_t*)mm_malloc(sizeof(list_t));
                if (!scope_defs) goto _forced_exit;
                list_init(scope_defs);
                stack_push(&scopes, (void*)scope_defs);
                break;
            }
            case LIR_ENDSCOPE: {
                list_t* scope_defs;
                if (!stack_pop(&scopes, (void**)&scope_defs)) break;
                foreach (symbol_id_t v_id, scope_defs) {
                    LIR_insert_block_before(LIR_create_block(LIR_VRDEALL, LIR_SUBJ_CONST(v_id), NULL, NULL), lh);
                }

                list_free(scope_defs);
                mm_free(scope_defs);
                break;
            }
            default: {
                list_t* scope_defs;
                if (!stack_top(&scopes, (void**)&scope_defs)) break;
                if (
                    LIR_is_writeop(lh->op) && 
                    lh->farg && lh->farg->t == LIR_VARIABLE
                ) list_add(scope_defs, (void*)lh->farg->storage.var.v_id);
                break;
            }
        }
    }

_forced_exit: {}
    stack_free_force_op(&scopes, (int (*)(void*))list_free);
    return 1;
}

/* Check if the provided variable is already deallocated.
Params:
    - `id` - Variable ID.
    - `bb` - Current Basic Block.

Returns 1 on success, otherwise 0. */
static int _already_deallocated(long id, cfg_block_t* bb) {
    lir_block_t* lh = bb->lmap.exit;
    while (lh) {
        if (lh->op == LIR_VRDEALL && lh->farg->storage.cnst.value == id) return 1;
        if (lh == bb->lmap.entry) break;
        lh = lh->prev;
    }

    return 0;
}

/* Deallocate variables / arrays / strings based on the USE, DEF, OUT and IN sets.
Params:
    - `cctx` - CFG context.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0. */
static int _use_def_pass(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        _scope_pass(fb);
        foreach (cfg_block_t* cb, &fb->blocks) {
            if (!cb->l && !cb->jmp) continue; /* Skip end blocks. There is no point to deallocate anything */

            set_t appeared;
            set_union(&appeared, &cb->curr_in, &cb->def);

            set_foreach (long vid, &appeared) {
                if (set_has(&cb->curr_out, (void*)vid)) continue;

                set_t owners;
                int hasown = 0;
                /* Check if this variable has an owner that is appearing
                   in the code further. */
                if (ALLIAS_get_owners(vid, &owners, &smt->m)) {
                    set_foreach (long svid, &owners) {
                        if (set_has(&cb->curr_out, (void*)svid)) {
                            hasown = 1;
                            break;
                        }
                    }
                }

                set_free(&owners);
                if (hasown) continue;
                if (!_already_deallocated(vid, cb)) {
                    LIR_insert_block_before(
                        LIR_create_block(LIR_VRDEALL, LIR_SUBJ_CONST(vid), NULL, NULL), 
                        cb->lmap.exit
                    );
                }

                map_foreach (allias_t* al, &smt->m.allias) {
                    if (!set_has(&al->owners, (void*)vid)) continue;
                    if (ALLIAS_mark_owner(al->v_id, vid, &smt->m)) {
                        if (!_already_deallocated(al->v_id, cb)) {
                            LIR_insert_block_before(
                                LIR_create_block(LIR_VRDEALL, LIR_SUBJ_CONST(al->v_id), NULL, NULL), 
                                cb->lmap.exit
                            );
                        }

                        set_free_force(&al->delown);
                        set_init(&al->delown, SET_NO_CMP);
                    }

                    break;
                }
            }

            set_free(&appeared);
        }
    }

    return 1;
}

int LIR_DFG_create_deall(cfg_ctx_t* cctx, sym_table_t* smt) {
    return _use_def_pass(cctx, smt);
}