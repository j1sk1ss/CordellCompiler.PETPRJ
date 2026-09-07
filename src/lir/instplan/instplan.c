#include <lir/instplan/instplan.h>

/* Simple function for DAG creation.
Params:
    - lh - Base lir block instruction.

Return instructions_dag_node_t* or NULL. */
static instructions_dag_node_t* _create_dag_node(lir_block_t* lh) {
    instructions_dag_node_t* nd = (instructions_dag_node_t*)mm_malloc(sizeof(instructions_dag_node_t));
    if (!nd) return NULL;
    set_init(&nd->vert, SET_NO_CMP);
    set_init(&nd->users, SET_NO_CMP);
    nd->b = lh;
    return nd;
}

/* Finds existed DAG node or creates a new one.
Note: It tries to find an existed DAG first, then if it doesn't, creates a new one.
Params:
    - lh - Base lir block instruction.
    - dag - DAG context.

Return instructions_dag_node_t* or NULL. */
static instructions_dag_node_t* _find_or_create_node(lir_block_t* lh, instructions_dag_t* dag) {
    if (!lh) return NULL;
    instructions_dag_node_t* prev;
    if (map_get(&dag->alive_edges, (long)lh, (void**)&prev)) {
        return prev;
    }

    instructions_dag_node_t* nd = _create_dag_node(lh);
    if (!nd) return NULL;

    map_put(&dag->alive_edges, (long)lh, nd);
    return nd;
}

/* Start to search the first LIR_CMP command from the 'lh' point.
Params:
    - lh - Search start point.
    - exit - Search exit point.

Return NULL if there is no LIR_CMP instruction. */
static lir_block_t* _find_first_cmp(lir_block_t* lh, lir_block_t* exit) {
    while (lh) {
        if (lh->op == LIR_CMP) return lh;
        if (lh == exit) break;
        lh = lh->prev;
    }

    return NULL;
}

/* Start to search the command with the 'trg' as a first argument.
Params:
    - lh - Search start point.
    - exit - Search exit point.
    - trg - Target first argument.

Return NULL if there is no command with a first argument equal to 'trg'. */
static lir_block_t* _find_src(lir_block_t* lh, lir_block_t* exit, lir_subject_t* trg) {
    while (lh) {
        if (lh->farg && LIR_subj_equals(lh->farg, trg)) return lh;
        if (lh == exit) break;
        lh = lh->prev;
    }

    return NULL;
} 

/* Link 'sub' to the source lir instruction.
Note: This function will try to find a source instruction for 'sub' and then will link them.
Params:
    - lh - Current location in BaseBlock.
    - sub - Target subject for the linking process.
    - bb - Current BaseBlock.
    - dag - Instruction's DAG context. */
static void _link_subject_to_source(
    lir_block_t* lh, lir_subject_t* sub, cfg_block_t* bb, instructions_dag_t* dag
) {
    lir_block_t* src_base = _find_src(lh, bb->lmap.entry, sub);
    if (!src_base) return;

    instructions_dag_node_t* src  = _find_or_create_node(_find_src(lh, bb->lmap.entry, sub), dag);
    instructions_dag_node_t* inst = _find_or_create_node(lh, dag);
    if (src && inst) {
        set_add(&inst->vert, src);
        set_add(&src->users, inst);
    }
}

/* Build an instruction DAG for the input BaseBlock.
Note: This function doesn't work outside of the 'bb' BaseBlock.
Params:
    - bb - Current BaseBlock.
    - dag - Instruction DAG context.

Returns 1 if DAG construction succeeds. */
static int _build_instructions_dag(cfg_block_t* bb, instructions_dag_t* dag) {
    iterate_lir_instructions (bb) {
        switch (lh->op) {
            case LIR_ARRDECL: {
                foreach (lir_subject_t* elem, &lh->targ->storage.list.h) {
                    if (elem->t != LIR_VARIABLE) continue;
                    _link_subject_to_source(lh, elem, bb, dag);
                }

                break;
            }
            case LIR_LOADFARG:
            case LIR_LOADFRET:
            case LIR_STARGLD: {
                _find_or_create_node(lh, dag);
                break;
            }
            case LIR_iMOV:
            case LIR_TI64: case LIR_TI32: case LIR_TI16: case LIR_TI8:
            case LIR_TU64: case LIR_TU32: case LIR_TU16: case LIR_TU8:
            case LIR_TF64: case LIR_TF32:
            case LIR_REF_GDREF: case LIR_REF:
            case LIR_GDREF: case LIR_LDREF:  _link_subject_to_source(lh, lh->sarg, bb, dag); break;
            case LIR_VRUSE:
            case LIR_STSARG:
            case LIR_STFARG: _link_subject_to_source(lh, lh->farg, bb, dag); break;
            case LIR_JE:
            case LIR_JNE: {
                instructions_dag_node_t* src  = _find_or_create_node(_find_first_cmp(lh, bb->lmap.entry), dag);
                instructions_dag_node_t* inst = _find_or_create_node(lh, dag);
                if (src && inst) {
                    set_add(&inst->vert, src);
                    set_add(&src->users, inst);
                }

                break;
            }
            case LIR_SYSC:
            case LIR_ECLL:
            case LIR_FCLL: {
                lir_block_t* (*finder)(lir_block_t *, lir_block_t *, int);
                if (lh->op == LIR_SYSC) finder = LIR_planner_get_next_sysc_abi;
                else                    finder = LIR_planner_get_next_func_abi;

                int reg_num = 0;
                lir_block_t* abi_reg;
                instructions_dag_node_t* inst = _find_or_create_node(lh, dag);
                if (!inst) break;

                while ((abi_reg = finder(lh->prev, bb->lmap.entry, reg_num++))) {
                    instructions_dag_node_t* src = _find_or_create_node(abi_reg, dag);
                    if (src) {
                        set_add(&inst->vert, src);
                        set_add(&src->users, inst);
                    }
                }

                lir_block_t* fres = LIR_planner_get_func_res(lh, bb->lmap.exit);
                if (fres) {
                    instructions_dag_node_t* res = _find_or_create_node(fres, dag);
                    set_add(&res->vert, inst);
                    set_add(&inst->users, res);
                    lh = lh->next;
                }

                break;
            }
            case LIR_bOR:  case LIR_CMP:  case LIR_iLWR:
            case LIR_iLRE: case LIR_iLRG: case LIR_iLGE:
            case LIR_iCMP: case LIR_iNMP: case LIR_iDIV:
            case LIR_iMOD: case LIR_bXOR: case LIR_bAND:
            case LIR_iMUL: case LIR_iSUB: case LIR_iADD: {
                lir_block_t* rax = _find_src(lh, bb->lmap.entry, lh->farg);
                lir_block_t* rbx = _find_src(lh, bb->lmap.entry, lh->sarg);
                instructions_dag_node_t* rax_nd = _find_or_create_node(rax, dag);
                instructions_dag_node_t* rbx_nd = _find_or_create_node(rbx, dag);

                instructions_dag_node_t* inst = _find_or_create_node(lh, dag);
                if (!inst) break;

                if (rax_nd) {
                    set_add(&inst->vert, rax_nd);
                    set_add(&rax_nd->users, inst);
                }

                if (rbx_nd) {
                    set_add(&inst->vert, rbx_nd);
                    set_add(&rbx_nd->users, inst);
                }

                break;
            }

            default: break;
        }
    }

    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        nd->remaining_deps = set_size(&nd->vert);
    }

    return 1;
}

static int _unload_dag(instructions_dag_t* dag) {
    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        set_free(&nd->vert);
        set_free(&nd->users);
    }

    return map_free_force(&dag->alive_edges);
}

static int _dag_toposort(instructions_dag_t* dag, list_t* sorted) {
    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        nd->indegree = 0;
    }

    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        set_foreach (instructions_dag_node_t* pred, &nd->vert) {
            pred->indegree++;
        }
    }

    list_t queue;
    list_init(&queue);
    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        if (!nd->indegree) {
            list_push_back(&queue, nd);
        }
    }

    instructions_dag_node_t* nd;
    while (list_size(&queue)) {
        nd = list_pop_front(&queue);
        list_push_back(sorted, nd);
        set_foreach (instructions_dag_node_t* succ, &nd->vert) {
            succ->indegree--;
            if (!succ->indegree) {
                list_push_back(&queue, succ);
            }
        }
    }

    list_free(&queue);
    return 1;
}

static int _compute_critical_path(target_info_t* trginfo, instructions_dag_t* dag) {
    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        op_info_t* opinfo;
        if (map_get(&trginfo->info, nd->b->op, (void**)&opinfo)) {
            nd->critical_path = opinfo->latency;
        }
    }

    list_t sorted;
    list_init(&sorted);
    _dag_toposort(dag, &sorted);
    
    foreach (instructions_dag_node_t* node, &sorted) {
        set_foreach (instructions_dag_node_t* dep, &node->vert) {
            op_info_t* opinfo;
            if (map_get(&trginfo->info, dep->b->op, (void**)&opinfo)) {
                int cand = dep->critical_path + opinfo->latency;
                if (cand > node->critical_path) node->critical_path = cand;
            }
        }
    }

    list_free(&sorted);
    return 1;
}

static instructions_dag_node_t* _select_best_node(list_t* ready_list) {
    if (!list_size(ready_list)) return NULL;
    instructions_dag_node_t* best = (instructions_dag_node_t*)list_get_head(ready_list);
    int best_cp = best->critical_path;
    foreach (instructions_dag_node_t* nd, ready_list) {
        if (nd == best) continue;
        if (nd->critical_path > best_cp) {
            best_cp = nd->critical_path;
            best = nd;
        }
    }

    return best;
}

static int _apply_schedule(cfg_block_t* bb, list_t* scheduled) {
    if (!bb || !list_size(scheduled)) return 0;
    lir_block_t* prev = NULL;
    foreach (instructions_dag_node_t* nd, scheduled) {
        if (prev) {
            if (bb->lmap.entry == nd->b) bb->lmap.entry = prev;
            if (bb->lmap.exit == prev)   bb->lmap.exit = nd->b;
            LIR_unlink_block(nd->b);
            LIR_insert_block_after(nd->b, prev);
        }

        prev = nd->b;
    }

    return 1;
}

static int _schedule_block(cfg_block_t* bb, instructions_dag_t* dag, target_info_t* trginfo) {
    _compute_critical_path(trginfo, dag);

    list_t ready_list;
    list_init(&ready_list);

    map_foreach (instructions_dag_node_t* nd, &dag->alive_edges) {
        if (!set_size(&nd->vert)) {
            list_push_back(&ready_list, nd);
        }
    }

    list_t scheduled;
    list_init(&scheduled);

    while (list_size(&ready_list)) {
        instructions_dag_node_t* best = _select_best_node(&ready_list);
        list_push_back(&scheduled, best);
        list_remove(&ready_list, best);
        set_foreach (instructions_dag_node_t* child, &best->users) {
            child->remaining_deps--;
            if (!child->remaining_deps) {
                list_push_back(&ready_list, child);
            }
        }
    }

    _apply_schedule(bb, &scheduled);
    list_free(&scheduled);
    list_free(&ready_list);
    return 1;
}

int LIR_plan_instructions(cfg_ctx_t* cctx, target_info_t* trginfo) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* bb, &fb->blocks) {
            instructions_dag_t dag;
            map_init(&dag.alive_edges, MAP_NO_CMP);
            _build_instructions_dag(bb, &dag);
            _schedule_block(bb, &dag, trginfo);
            _unload_dag(&dag);
        }
    }
    
    return 1;
}
