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
        annots.is_like_c ? SMT_NULL : annots.align, !annots.is_union, annots.is_vtable, &smt->t
    );
    name->t->t_type  = CUSTOM_TYPE_TOKEN;

    forward_token(it, 1);
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
