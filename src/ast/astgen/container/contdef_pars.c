#include <ast/astgen/astgen.h>

DEFINE_PARSER(cpl_parse_contdef, {
    annotations_summary_t annots = { 
        .align  = CONF_get_full_bytness(), .section = NULL, 
        .salign = SMT_NULL,                .reg     = SMT_NULL 
    };
    
    ANNOT_read_annotations(&ctx->annots, &annots);

    ast_node_t* base = AST_create_node(CURRENT_TOKEN);
    PARSER_ASSERT(!base, NULL, "Can't create a base for a container!");

    forward_token(it, 1);
    ast_node_t* name = AST_create_node(CURRENT_TOKEN);
    PARSER_ASSERT_DO(
        !name, "Can't create a name for a container!", 
        { AST_unload(base); ANNOT_destroy_summary(&annots); }
    );

    AST_add_node(base, name);
    stack_top(&ctx->scopes.stack, (void**)&name->sinfo.s_id);
    name->sinfo.t_id = TPTB_add_info(
        name->t->body, name->sinfo.s_id, TYPE_CUSTOM, 
        annots.is_like_c ? SMT_NULL : annots.align, 
        !annots.is_union, annots.is_vtable, base->t->t_type == INTERFACE_TOKEN, 
        &smt->t
    );
    name->t->t_type = CUSTOM_TYPE_TOKEN;

    if (consume_token(it, STAT_TOKEN)) {
        forward_token(it, 1);
        do {
            symbol_id_t base_tid = type_lookup(CURRENT_TOKEN, ctx, smt);
            PARSER_ASSERT_DO(
                base_tid == NO_SYMBOL_ID, "Can't find inheretence base!",
                { AST_unload(base); ANNOT_destroy_summary(&annots); }
            );

            type_info_t base_ti;
            if (TPTB_get_info_id(base_tid, &base_ti, &smt->t) && base_ti.t == TYPE_CUSTOM) {
                if (base_ti.body.custom.layout.vtable) TPTB_enable_vtable(name->sinfo.t_id, &smt->t);
                foreach (symbol_id_t c_id, &base_ti.body.custom.layout.children) {
                    type_info_t ch_ti;
                    member_info_t ch_mi;
                    if (
                        TPTB_get_info_id(c_id, &ch_ti, &smt->t) &&
                        TPTB_get_member_info(base_tid, c_id, NULL, &ch_mi, &smt->t) &&
                        ch_ti.t == TYPE_METHOD && ch_ti.body.method.in_vtable
                    ) TPTB_add_as_child(
                        name->sinfo.t_id, TPTB_add_copy(ch_ti.id, ch_ti.ptr, &smt->t),
                        ch_mi.name, FIELD_NO_CHANGE, &smt->t
                    );
                }
            }
        } while (consume_token(it, COMMA_TOKEN));
    }

    stack_push(&ctx->types, (void*)name->sinfo.t_id);
    ast_node_t* decls = cpl_parse_scope(it, ctx, smt, 1);
    stack_pop(&ctx->types, NULL);

    PARSER_ASSERT_DO(
        !decls, "Can't parse the container's body!", 
        { AST_unload(base); ANNOT_destroy_summary(&annots); }
    );
    
    TPTB_set_child_scope_id(name->sinfo.t_id, decls->sinfo.s_id, &smt->t);
    AST_add_node(base, decls);
    ANNOT_destroy_summary(&annots);
    return base;
})
