/* dfg.c - Compute all stuff for liveness analysis.
IN  - Live variables from previous blocks
OUT - Live variables after this block
DEF - All new variables that defined first time
USE - All variables that has been readed by someone

IN  = union(USE, (OUT - DEF))
OUT = union(IN successors) */

#include <lir/dfg.h>

static int _add_vars_from_subject(set_t* s, lir_subject_t* subj) {
    if (!subj) return 0;
    switch (subj->t) {
        case LIR_VARIABLE: set_add(s, (void*)subj->storage.var.v_id); break;
        case LIR_ARGLIST: {
            foreach (lir_subject_t* arg, &subj->storage.list.h) {
                if (arg && arg->t == LIR_VARIABLE) {
                    set_add(s, (void*)arg->storage.var.v_id);
                }
            }
         
            break;
        }
        default: break;
    }

    return 1;
}

static int _lir_inst_usedef(lir_block_t* lh, set_t* use, set_t* def) {
    set_init(use, SET_CMP);
    set_init(def, SET_CMP);
    if (!lh || lh->unused) return 0;
    if (lh->op == LIR_phiMOV) {
        _add_vars_from_subject(def, lh->farg);
        _add_vars_from_subject(use, lh->sarg);
        return 0;
    }

    iterate_lir_args (lir_subject_t* arg, lh, LIR_is_writeop(lh->op)) {
        _add_vars_from_subject(use, arg);
    }

    if (LIR_is_writeop(lh->op)) {
        _add_vars_from_subject(def, lh->farg);
    }

    return 1;
}

int LIR_DFG_compute_usedef(cfg_ctx_t* cctx) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* bb, &fb->blocks) {
            set_free(&bb->use);
            set_free(&bb->def);
            set_init(&bb->use, SET_CMP);
            set_init(&bb->def, SET_CMP);

            iterate_lir_instructions (bb) {
                set_t inst_use, inst_def, real_use;

                _lir_inst_usedef(lh, &inst_use, &inst_def);

                set_copy(&real_use, &inst_use);
                set_minus_set(&real_use, &bb->def);

                set_t next_use, next_def;
                set_union(&next_use, &bb->use, &real_use);
                set_union(&next_def, &bb->def, &inst_def);

                set_free(&bb->use);
                set_free(&bb->def);
                bb->use = next_use;
                bb->def = next_def;

                set_free(&real_use);
                set_free(&inst_use);
                set_free(&inst_def);
            }
        }
    }

    return 1;
}

/* Compute the 'OUT' set during the liveness analysis.
Params:
    - `cfg` - CFG context.

Returns 1 if succeeds. */
static int _compute_out(cfg_block_t* cfg) {
    set_t out;
    set_init(&out, SET_CMP);
    if (cfg->l) {
        set_t next_out;
        set_union(&next_out, &out, &cfg->l->curr_in);
        set_free(&out);
        out = next_out;
    }

    if (cfg->jmp) {
        set_t next_out;
        set_union(&next_out, &out, &cfg->jmp->curr_in);
        set_free(&out);
        out = next_out;
    }
    
    set_free(&cfg->curr_out);
    set_copy(&cfg->curr_out, &out);
    set_free(&out);
    return 1;
}

/* Compute the 'IN' set during the liveness analysis.
Params:
    - `cfg` - CFG context.

Returns 1 if succeeds. */
static int _compute_in(cfg_block_t* cfg) {
    set_t tmp;
    set_copy(&tmp, &cfg->curr_out);
    set_minus_set(&tmp, &cfg->def);
    set_free(&cfg->curr_in);
    set_union(&cfg->curr_in, &cfg->use, &tmp);
    set_free(&tmp);
    return 1;
}

int LIR_DFG_compute_inout(cfg_ctx_t* cctx) {
    LIR_DFG_compute_usedef(cctx);
    foreach (cfg_func_t* fb, &cctx->funcs) {
        while (1) {
            list_iter_t bit;
            list_iter_tinit(&fb->blocks, &bit);
            cfg_block_t* cb;
            while ((cb = (cfg_block_t*)list_iter_prev(&bit))) {
                _compute_out(cb);
                _compute_in(cb);
            }

            int same = 1;
            list_iter_tinit(&fb->blocks, &bit);
            while ((cb = (cfg_block_t*)list_iter_prev(&bit))) {
                if (
                    !set_equal(&cb->curr_in, &cb->prev_in) || 
                    !set_equal(&cb->curr_out, &cb->prev_out)
                ) {
                    same = 0;
                    break;
                }
            }

            if (same) break;
            list_iter_tinit(&fb->blocks, &bit);
            while ((cb = (cfg_block_t*)list_iter_prev(&bit))) {
                set_free(&cb->prev_in);
                set_copy(&cb->prev_in, &cb->curr_in);
                set_free(&cb->prev_out);
                set_copy(&cb->prev_out, &cb->curr_out);
            }
        }
    }

    return 1;
}

typedef struct {
    lir_block_t     p;
    lir_registers_t dst_reg;
    lir_registers_t src_reg;
    char            done : 1;
} phi_copy_t;

/* Save the mutable payload of a LIR block into temporary storage.
Params:
    - `p` - Destination payload storage.
    - `lh` - Source LIR block. */
static inline void _save_payload(lir_block_t* p, lir_block_t* lh) {
    p->op     = lh->op;
    p->farg   = lh->farg;
    p->sarg   = lh->sarg;
    p->targ   = lh->targ;
    p->unused = lh->unused;
}

/* Restore the mutable payload of a LIR block from temporary storage.
Params:
    - `lh` - Destination LIR block.
    - `p` - Source payload storage. */
static inline void _load_payload(lir_block_t* lh, lir_block_t* p) {
    lh->op     = p->op;
    lh->farg   = p->farg;
    lh->sarg   = p->sarg;
    lh->targ   = p->targ;
    lh->unused = p->unused;
}

/* Resolve the register assigned to a LIR subject.
Params:
    - `s` - LIR subject to inspect.
    - `colors` - Map from variable ids to allocated registers.
    - `reg` - Output register.

Returns 1 if register was resolved, otherwise 0. */
static int _subj_reg(lir_subject_t* s, map_t* colors, lir_registers_t* reg) {
    if (!s) return 0;
    if (s->t == LIR_VARIABLE) {
        long color;
        if (!map_get(colors, s->storage.var.v_id, (void**)&color)) return 0;
        *reg = color;
        return 1;
    }

    if (s->t == LIR_REGISTER) {
        *reg = s->storage.reg.reg;
        return 1;
    }

    return 0;
}

/* Check whether a copy destination is still needed as a source by another
pending phi copy.
Params:
    - `copies` - Phi copy array.
    - `n` - Number of phi copies.
    - `i` - Copy index to check.

Returns 1 if destination register is used as a pending source, otherwise 0. */
static int _dst_used_as_src(phi_copy_t* copies, int n, int i) {
    for (int j = 0; j < n; j++) {
        if (i == j || copies[j].done) continue;
        if (copies[i].dst_reg == copies[j].src_reg) return 1;
    }

    return 0;
}

/* Sort an array of phi moves so each move is emitted after all reads of its
destination register are finished.
Params:
    - `nodes` - Phi move nodes to reorder.
    - `n` - Number of nodes.
    - `colors` - Map from variable ids to allocated registers.

Returns 1 if sorting succeeds, otherwise 0. */
static int _sort_phi_array(lir_block_t** nodes, int n, map_t* colors) {
    phi_copy_t* copies = (phi_copy_t*)mm_malloc(sizeof(phi_copy_t) * n);
    if (!copies) return 0;
    for (int i = 0; i < n; i++) {
        _save_payload(&copies[i].p, nodes[i]);
        copies[i].done = 0;
        if (!_subj_reg(nodes[i]->farg, colors, &copies[i].dst_reg)) {
            copies[i].dst_reg = -1;
        }

        if (!_subj_reg(nodes[i]->sarg, colors, &copies[i].src_reg)) {
            copies[i].src_reg = -1;
        }
    }

    int pos = 0;
    while (pos < n) {
        int found = -1;
        for (int i = 0; i < n; i++) {
            if (copies[i].done) continue;
            if (copies[i].dst_reg == copies[i].src_reg) {
                found = i;
                break;
            }

            if (!_dst_used_as_src(copies, n, i)) {
                found = i;
                break;
            }
        }

        if (found < 0) {
            mm_free(copies);
            return 0;
        }

        _load_payload(nodes[pos], &copies[found].p);
        copies[found].done = 1;
        pos++;
    }

    mm_free(copies);
    return 1;
}

/* Sort a contiguous group of phi moves.
Params:
    - `first` - First phi move in the group.
    - `last` - Block after the last phi move.
    - `colors` - Map from variable ids to allocated registers.

Returns 1 if sorting succeeds, otherwise 0. */
static int _sort_phi_group(lir_block_t* first, lir_block_t* last, map_t* colors) {
    if (!first || first == last) return 1;

    int n = 0;
    lir_block_t* lh = first;
    while (lh && lh != last && lh->op == LIR_phiMOV && !lh->unused) {
        lir_block_t* next = LIR_get_next(lh, last, 1);
        if (next == lh) return 0;
        lh = next;
        n++;
    }

    if (n <= 1) return 1;
    lir_block_t** nodes = (lir_block_t**)mm_malloc(sizeof(lir_block_t*) * n);
    if (!nodes) return 0;

    lh = first;
    for (int i = 0; i < n; i++) {
        nodes[i] = lh;
        lh = LIR_get_next(lh, last, 1);
    }

    int ok = _sort_phi_array(nodes, n, colors);
    mm_free(nodes);
    return ok;
}

int LIR_RA_sort_phi_movs(cfg_ctx_t* cctx, map_t* colors) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* cb, &fb->blocks) {
            lir_block_t* lh = LIR_get_next(cb->lmap.entry, cb->lmap.exit, 0);
            while (lh && lh != cb->lmap.exit) {
                if (lh->op != LIR_phiMOV || lh->unused) {
                    lh = LIR_get_next(lh, cb->lmap.exit, 1);
                    continue;
                }

                lir_block_t* first = lh;
                lir_block_t* after = lh;
                while (
                    after &&
                    after != cb->lmap.exit &&
                    after->op == LIR_phiMOV &&
                    !after->unused
                ) after = LIR_get_next(after, cb->lmap.exit, 1);
                if (first != after && !_sort_phi_group(first, after, colors)) {
                    return 0;
                }

                lh = after;
            }
        }
    }

    return 1;
}
