#include <symtab/functb.h>

int FNTB_get_info_id(symbol_id_t id, func_info_t* out, functab_ctx_t* ctx) {
    print_log("FNTB_get_info(id=%li)", id);
    func_info_t* fi;
    if (map_get(&ctx->functb, id, (void**)&fi)) {
        if (out) str_memcpy(out, fi, sizeof(func_info_t));
        return 1;
    }

    return 0;
}

int FNTB_collect_info(string_t* fname, symbol_id_t s_id, list_t* out, functab_ctx_t* ctx, scopetab_ctx_t* sctx) {
    print_log("FNTB_collect_info(name=%s, s_id=%li)", fname ? fname->body : "(null)", s_id);
    for (symbol_id_t visible_sid = s_id; visible_sid != NO_SYMBOL_ID; visible_sid = SCPTB_get_parent(visible_sid, sctx)) {
        map_foreach (func_info_t* fi, &ctx->functb) {
            if (fi->flags.generic) continue;
            if (
                fi->s_id == visible_sid &&
                fi->name->equals(fi->name, fname)
            ) list_add(out, fi);
        }
    }

    print_debug("FNTB_collect_info(%s) -> %i!\n", fname ? fname->body : "(null)", list_size(out));
    return 1;
}

int FNTB_get_info(string_t* fname, symbol_id_t s_id, func_info_t* out, functab_ctx_t* ctx) {
    print_log("FNTB_get_info(name=%s, s_id=%li)", fname ? fname->body : "(null)", s_id);
    map_foreach (func_info_t* fi, &ctx->functb) {
        if (fi->name->equals(fi->name, fname) && (fi->s_id == s_id)) {
            if (out) str_memcpy(out, fi, sizeof(func_info_t));
            return 1;
        }
    }

    print_warn("FNTB_get_info -> NF!");
    return 0;
}

static func_info_t* _create_func_info(string_t* name, func_info_flags_t flags, ast_node_t* args, ast_node_t* rtype) {
    func_info_t* fn = (func_info_t*)mm_malloc(sizeof(func_info_t));
    if (!fn) return NULL;
    str_memset(fn, 0, sizeof(func_info_t));
    list_init(&fn->local);
    list_init(&fn->template.resolutions);
    list_init(&fn->template.registered_types);
    map_init(&fn->template.generic, MAP_CMP);
    if (name) fn->name = name->copy(name);
    fn->args  = AST_copy_node(args, 0, 0, 1, args->siblings.t);
    fn->rtype = AST_copy_node(rtype, 0, 0, 1, NULL);
    fn->flags = flags;
    return fn;
}

static int _is_function_presented(
    string_t* name, symbol_id_t s_id, ast_node_t* args, map_t* gen, func_info_t* out, functab_ctx_t* ctx
) {
    map_foreach (func_info_t* fi, &ctx->functb) {
        if (
            (s_id == FIELD_NO_CHANGE || fi->s_id == s_id)                                                  &&
            fi->name->equals(fi->name, name)                                                               &&
            AST_hash_node_stop(args->c, 1, SCOPE_TOKEN) == AST_hash_node_stop(fi->args->c, 1, SCOPE_TOKEN) &&
            (!gen || map_equals(&fi->template.generic, gen))
        ) {
            if (out) str_memcpy(out, fi, sizeof(func_info_t));
            return 1;
        }
    }

    return 0;
}

static string_t* _create_virt_name(symbol_id_t id, string_t* name) {
    string_t* virt  = name->copy(name);
    string_t* index = create_string_from_int(id);
    virt->cat(virt, index);
    destroy_string(index);
    return virt;
}

symbol_id_t FNTB_add_info(
    string_t* name, string_t* vname, func_info_flags_t flags, symbol_id_t s_id, ast_node_t* args, ast_node_t* rtype, functab_ctx_t* ctx
) {
    print_log(
        "FNTB_add_info(name=%s, global=%i, entry=%i, naked=%i, args=%lu)", 
        name ? name->body : "(null)", flags.global, flags.entry, flags.naked, args ? AST_hash_node_stop(args->c, 1, SCOPE_TOKEN) : 0
    );
    
    func_info_t out;
    if (_is_function_presented(name, s_id, args, NULL, &out, ctx)) return out.id; 

    func_info_t* nnd = _create_func_info(name, flags, args, rtype);
    if (!nnd) return 0;
    nnd->s_id       = s_id;
    nnd->flags.used = flags.global;
    
    nnd->id = ctx->curr_id++;
    if (!vname) nnd->virt = _create_virt_name(nnd->id, name);
    else nnd->virt = vname->copy(vname);

    map_put(&ctx->functb, nnd->id, nnd);
    return nnd->id;
}

symbol_id_t FNTB_add_copy(func_info_t* src, functab_ctx_t* ctx) {
    print_log("FNTB_add_copy(id=%llu)", src->id);
    func_info_t* nnd = _create_func_info(src->name, src->flags, src->args, src->rtype);
    if (!nnd) return NO_SYMBOL_ID;
    
    nnd->id   = ctx->curr_id++;
    nnd->virt = _create_virt_name(nnd->id, src->name);
    nnd->s_id = NO_SYMBOL_ID;

    foreach (symbol_id_t t_id, &src->template.registered_types) {
        list_add(&nnd->template.registered_types, (void*)t_id);
    }

    map_put(&ctx->functb, nnd->id, nnd);
    return nnd->id;
}

int FNTB_add_local(symbol_id_t f_id, symbol_id_t l_id, functab_ctx_t* ctx) {
    print_log("FNTB_add_local(id=%llu, local=%li)", f_id, l_id);
    func_info_t* fi;
    if (map_get(&ctx->functb, f_id, (void**)&fi)) {
        list_add(&fi->local, (void*)l_id);
        return 1;
    }

    return 0;
}

int FNTB_rewrite_flags(symbol_id_t id, func_info_flags_t flags, functab_ctx_t* ctx) {
    print_log("FNTB_rewrite_flags(id=%li)", id);
    func_info_t* fi;
    if (map_get(&ctx->functb, id, (void**)&fi)) {
        fi->flags = flags;
        return 1;
    }

    return 0;
}

int FNTB_update_func(
    symbol_id_t id, string_t* name, func_info_flags_t flags, ast_node_t* args, ast_node_t* rtype, functab_ctx_t* ctx
) {
    print_log("FNTB_update_func(id=%li, name=%s)", id, name->body);
    func_info_t* fi;
    if (map_get(&ctx->functb, id, (void**)&fi)) {
        if (name) {
            destroy_string(fi->name);
            destroy_string(fi->virt);
            fi->name = name->copy(name);
            fi->virt = _create_virt_name(id, name);
        }
        
        if (flags.global != FIELD_NO_CHANGE)   fi->flags.global   = flags.global;
        if (flags.local != FIELD_NO_CHANGE)    fi->flags.local    = flags.local;
        if (flags.entry != FIELD_NO_CHANGE)    fi->flags.entry    = flags.entry;
        if (flags.naked != FIELD_NO_CHANGE)    fi->flags.naked    = flags.naked;
        if (flags.vargs != FIELD_NO_CHANGE)    fi->flags.vargs    = flags.vargs;
        if (flags.used != FIELD_NO_CHANGE)     fi->flags.used     = flags.used;
        if (flags.external != FIELD_NO_CHANGE) fi->flags.external = flags.external;
        if (flags.self != FIELD_NO_CHANGE)     fi->flags.self     = flags.self;
        if (flags.abstract != FIELD_NO_CHANGE) fi->flags.abstract = flags.abstract;
        if (flags.override != FIELD_NO_CHANGE) fi->flags.override = flags.override;
        
        if (args) {
            AST_unload(fi->args);
            fi->args = AST_copy_node(args, 0, 0, 1, args->siblings.t);
        }

        if (rtype) {
            AST_unload(fi->rtype);
            fi->rtype = AST_copy_node(rtype, 0, 0, 1, NULL);
        }

        return 1;
    }

    return 0;
}

int FNTB_has_generic_types(symbol_id_t f_id, functab_ctx_t* ctx) {
    print_log("FNTB_has_generic_types(id=%llu)", f_id);
    func_info_t* fi;
    if (map_get(&ctx->functb, f_id, (void**)&fi) && list_size(&fi->template.registered_types)) {
        return 1;
    }

    return 0;
}

int FNTB_clear_generic_types(symbol_id_t f_id, functab_ctx_t* ctx) {
    print_log("FNTB_clear_generic_types(id=%llu)", f_id);
    func_info_t* fi;
    if (map_get(&ctx->functb, f_id, (void**)&fi)) {
        list_free(&fi->template.registered_types);
        list_init(&fi->template.registered_types);
        return 1;
    }

    return 0;
}

int FNTB_register_generic_type(symbol_id_t f_id, symbol_id_t t_id, functab_ctx_t* ctx) {
    print_log("FNTB_register_generic_type(id=%llu, t=%s)", f_id, t_id);
    func_info_t* fi;
    if (map_get(&ctx->functb, f_id, (void**)&fi)) {
        list_add(&fi->template.registered_types, (void*)t_id);
        return 1;
    }

    return 0;
}

static int _resolve_types(ast_node_t* node, symbol_id_t t_id, token_type_t t, functab_ctx_t* ctx) {
    if (!node) return 0;
    _resolve_types(node->siblings.n, t_id, t, ctx);
    _resolve_types(node->c, t_id, t, ctx);
    if (!node->t) return 0;
    if (
        node->t->t_type == GENERIC_TYPE_TOKEN &&
        node->sinfo.t_id == t_id
    ) node->t->t_type = t;
    else if (
        (node->t->t_type == CALLING_TOKEN && node->c->c) ||
        (node->t->t_type == CALL_ADDR_TOKEN && node->c)
    ) {
        list_t types;
        list_init(&types);
        ast_node_t* name = node->t->t_type == CALLING_TOKEN ? node->c : node;
        ast_node_t* type_node = name->c;
        while (type_node) {
            if (type_node->t->t_type == GENERIC_TYPE_TOKEN) {
                list_free(&types);
                return 0;
            }

            list_add(&types, (void*)type_node->t->t_type);
            type_node = type_node->siblings.n;
        }

        name->sinfo.v_id = FNTB_create_resolved_copy(name->sinfo.v_id, &types, ctx);
        list_free(&types);
    }

    return 1;
}

symbol_id_t FNTB_create_resolved_copy(symbol_id_t id, list_t* types, functab_ctx_t* ctx) {
    print_log("FNTB_create_resolved_copy(id=%llu, types_count=%i)", id, list_size(types));
    func_info_t* fi;
    if (map_get(&ctx->functb, id, (void**)&fi)) {
        func_info_t existed;
        ast_node_t *args = AST_copy_node(fi->args, 0, 0, 1, NULL), *rtype = AST_copy_node(fi->rtype, 0, 0, 1, NULL);

        map_t reg_types;
        map_init(&reg_types, MAP_CMP);

        token_type_t** flatten_types   = (token_type_t**)list_flatten(types);
        symbol_id_t** flatten_types_id = (symbol_id_t**)list_flatten(&fi->template.registered_types);
        for (int i = 0; i < MIN(list_size(types), list_size(&fi->template.registered_types)); i++) {
            _resolve_types(args, (symbol_id_t)flatten_types_id[i], (token_type_t)flatten_types[i], ctx);
            _resolve_types(rtype, (symbol_id_t)flatten_types_id[i], (token_type_t)flatten_types[i], ctx);
            map_put(&reg_types, flatten_types_id[i], (void*)flatten_types[i]);
        }

        mm_free(flatten_types);
        mm_free(flatten_types_id);
        
        if (_is_function_presented(fi->name, fi->s_id, args, &reg_types, &existed, ctx)) {
            map_free(&reg_types);
            AST_unload(args);
            AST_unload(rtype);
            return existed.id;
        }
        
        func_info_t* n   = _create_func_info(fi->name, fi->flags, args, rtype);
        n->flags.generic = 0;
        
        AST_unload(args);
        AST_unload(rtype);

        map_free(&n->template.generic);
        map_copy(&n->template.generic, &reg_types);
        map_free(&reg_types);

        n->s_id = fi->s_id;
        n->id   = ctx->curr_id++;
        n->virt = _create_virt_name(n->id, n->name);
        
        if (n->virt) {
            foreach (token_type_t* t, types) {
                n->virt->rcat(n->virt, "__");
                n->virt->rcat(n->virt, DUMP_format_token_type((token_type_t)t));
            }
        }

        map_put(&ctx->functb, n->id, n);
        list_add(&fi->template.resolutions, (void*)n->id);
        return n->id;
    }

    return 0;
}

static int _function_info_unload(func_info_t* info) {
    destroy_string(info->name);
    destroy_string(info->virt);
    list_free(&info->local);
    list_free(&info->template.resolutions);
    list_free(&info->template.registered_types);
    map_free(&info->template.generic);
    if (info->args)  AST_unload(info->args);
    if (info->rtype) AST_unload(info->rtype);
    return mm_free(info);
}

int FNTB_unload(functab_ctx_t* ctx) {
    print_log("FNTB_unload()");
    return map_free_force_op(&ctx->functb, (int (*)(void *))_function_info_unload);
}
