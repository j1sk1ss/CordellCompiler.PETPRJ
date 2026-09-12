#include <hir/func.h>

/* Create a call graph node.
Params:
    - `f_id` - Function ID.

Returns a new call graph node or NULL. */
static call_graph_node_t* _create_cgraph_node(symbol_id_t f_id) {
    call_graph_node_t* nd = (call_graph_node_t*)mm_malloc(sizeof(call_graph_node_t));
    if (!nd) return NULL;
    str_memset(nd, 0, sizeof(call_graph_node_t));
    nd->f_id = f_id;
    set_init(&nd->edges, SET_NO_CMP);
    return nd;
}

/* Create and register a new call node in the call context.
Params:
    - `f_id` - Function ID.
    - `ctx` - Call graph context.

Returns 1 on success, otherwise 0. */
static inline int _register_func(symbol_id_t f_id, call_graph_t* ctx) {
    call_graph_node_t* f = _create_cgraph_node(f_id);
    if (!f) return 0;
    return map_put(&ctx->verts, f_id, f);
}

/* Add vertex into the call graph context.
Connects two existed vertices in the call graph context. 
Params:
    - `src_id` - Source function ID.
    - `dst_id` - Destination function ID.
    - `ctx` - Call graph context.

Returns 1 if succeeds. */
static int _add_vert(symbol_id_t src_id, symbol_id_t dst_id, call_graph_t* ctx) {
    call_graph_node_t *src, *dst;
    if (
        !map_get(&ctx->verts, src_id, (void**)&src) || /* If the source ID isn't presented in the context      */
        !map_get(&ctx->verts, dst_id, (void**)&dst)    /* If the destination ID isn't presented in the context */
    ) return 0;
    set_add(&src->edges, dst);
    return 1;
}

/* Register all existed functions from the symtable.
Params:
    - `ctx` - Call graph context.
    - `smt` - Symtable.

Returns 1 if succeeds. */
static int _register_functions(call_graph_t* ctx, sym_table_t* smt) {
    list_init(&ctx->entries);
    map_foreach (func_info_t* fi, &smt->f.functb) {
        if (
            fi->flags.entry || (fi->flags.global && fi->flags.external != FNTB_SHALLOW_EXTERN)
        ) list_add(&ctx->entries, (void*)fi->id);
        _register_func(fi->id, ctx);
    }

    map_foreach (array_info_t* ai, &smt->a.arrtb) {
        foreach (array_elem_info_t* elem, &ai->elems) {
            if (elem->t == ARRAY_ELEM_FUNC_TYPE && elem->s.f_id != NO_SYMBOL_ID) {
                list_add(&ctx->entries, (void*)elem->s.f_id);
            }
        }
    }

    return 1;
}

/* Traverse the list of functions, connect each function with called functions.
Params:
    - `cctx` - CFG context.
    - `ctx` - Call graph context.

Returns 1 on success, otherwise 0. */
static int _connect_edges(cfg_ctx_t* cctx, call_graph_t* ctx) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        foreach (cfg_block_t* bb, &fb->blocks) {
            iterate_hir_instructions (bb) {
                if (hh->unused) continue;
                if (
                    HIR_is_funccall(hh->op) ||                      /* Direct function call           */
                    (hh->op == HIR_REF && hh->sarg->t == HIR_FNAME) /* Taking a pointer of a function */
                ) _add_vert(fb->f_id, hh->sarg->storage.str.s_id, ctx);
            }
        }
    }

    return 1;
}

int HIR_CG_build(cfg_ctx_t* cctx, call_graph_t* ctx, sym_table_t* smt) {
    map_init(&ctx->verts, MAP_NO_CMP);
    _register_functions(ctx, smt);
    return _connect_edges(cctx, ctx);
}

/* Try to reach all possible call graph nodes from the current node.
Params:
    - `nd` - Current call graph node.
    - `ctx` - Call graph context.

Returns 1 if succeeds. */
static int _mark_block(call_graph_node_t* nd, call_graph_t* ctx) {
    nd->flag = 1;
    set_foreach (call_graph_node_t* nnd, &nd->edges) {
        if (nd->f_id == nnd->f_id) continue;
        _mark_block(nnd, ctx);
    }
    
    return 1;
}

int HIR_CG_perform_dfe(call_graph_t* ctx, sym_table_t* smt) {
    foreach (symbol_id_t e_fid, &ctx->entries) {
        call_graph_node_t* entry;
        if (!map_get(&ctx->verts, e_fid, (void**)&entry)) continue;
        _mark_block(entry, ctx);
        map_foreach (call_graph_node_t* nd, &ctx->verts) {
            func_info_t fi;
            if (!FNTB_get_info_id(nd->f_id, &fi, &smt->f)) continue;
            FNTB_update_func(nd->f_id, FNTB_ONLY_FLAGS(FNTB_SET_USED((fi.flags.global && fi.flags.external != FNTB_SHALLOW_EXTERN) ? 1 : nd->flag)), &smt->f);
        }
    }

    return 1;
}

int HIR_CG_apply_dfe(cfg_ctx_t* cctx, sym_table_t* smt) {
    foreach (cfg_func_t* fb, &cctx->funcs) {
        func_info_t fi;
        if (!FNTB_get_info_id(fb->f_id, &fi, &smt->f)) {
            fb->used = 0;
            continue;
        }
        
        fb->used = fi.flags.used;
    }

    return 1;
}

int HIR_CG_unload(call_graph_t* ctx) {
    map_foreach (call_graph_node_t* node, &ctx->verts) {
        set_free(&node->edges);
    }

    list_free(&ctx->entries);
    return map_free_force(&ctx->verts);
}
