#include <ast/astgen/astgen.h>

int cpl_parse_funcdef_args(PARSER_ARGS) {
    PARSER_ARGS_USE;
    SAVE_TOKEN_POINT;
    
    ast_node_t* trg = (ast_node_t*)carry;
    while (CURRENT_TOKEN && CURRENT_TOKEN->t_type != CLOSE_BRACKET_TOKEN) {
        ast_node_t* arg = NULL;
        symbol_id_t arg_type = type_lookup(CURRENT_TOKEN, ctx, smt);
        if (
            TKN_is_builtin_type(CURRENT_TOKEN) || 
            arg_type != NO_SYMBOL_ID           || 
            CURRENT_TOKEN->t_type == SIGNATURE_TOKEN
        ) arg = cpl_parse_variable_declaration(it, ctx, smt, arg_type);
        else if (CURRENT_TOKEN->t_type == VAR_ARGUMENTS_TOKEN) {
            arg = AST_create_node(CURRENT_TOKEN);
            forward_token(it, 1);
        }
        else if (CURRENT_TOKEN->t_type == ANNOTATION_TOKEN) {
            cpl_parse_annot(it, ctx, smt, carry);
            forward_token(it, 1);
            continue;
        }
        else {
            PARSE_ERROR("Error during the argument parsing! Unknown token=%i!", CURRENT_TOKEN->t_type);
            RESTORE_TOKEN_POINT;
            return 0;
        }

        if (arg) AST_add_node(trg, arg);
        else {
            PARSE_ERROR("Error during the argument parsing! (<type> <name>)!");
            RESTORE_TOKEN_POINT;
            return 0;
        }

        if (CURRENT_TOKEN->t_type == COMMA_TOKEN) {
            forward_token(it, 1);
        }
    }

    return 1;
}

DEFINE_PARSER(cpl_parse_function, {
    ast_node_t* base = AST_create_node(CURRENT_TOKEN);
    PARSER_ASSERT(!base, NULL, "Can't create a base for the function!");
    PARSER_ASSERT(!consume_token(it, UNKNOWN_STRING_TOKEN), base, "Expected 'UNKNOWN_STRING_TOKEN' token!");

    string_t* base_type = NULL;
    ast_node_t* name = AST_create_node(CURRENT_TOKEN);
    PARSER_ASSERT(!name, base, "Can't create a base for the function's name!");
    
    if (
        consume_token(it, STAT_TOKEN) && 
        consume_token(it, UNKNOWN_STRING_TOKEN)
    ) {
        base_type = name->t->body->copy(name->t->body);
        AST_unload(name);
        name = AST_create_node(CURRENT_TOKEN);
        forward_token(it, 1);
    }
    
    PARSER_ASSERT(!name, base, "Can't create a base for the function's name!");
    name->t->t_type = FUNC_NAME_TOKEN;
    AST_add_node(base, name);

    stack_top(&ctx->scopes.stack, (void**)&name->sinfo.s_id);
    symbol_id_t args_scope = SCPTB_push_scope(&smt->sc, &ctx->scopes.stack);

    list_t generic_types;
    list_init(&generic_types);

    switch (CURRENT_TOKEN->t_type) {
        case OPEN_BRACKET_TOKEN: break;
        case LOWER_TOKEN: {
            forward_token(it, 1);
            do {
                symbol_id_t t_id = TPTB_add_info(CURRENT_TOKEN->body, args_scope, TYPE_GENERICS, SMT_NULL, 0, 0, 0, &smt->t);
                if (t_id != NO_SYMBOL_ID) list_add(&generic_types, (void*)t_id);
                if (consume_token(it, COMMA_TOKEN)) forward_token(it, 1);
            } while (CURRENT_TOKEN->t_type != LARGER_TOKEN);
            forward_token(it, 1);
            break;
        }
        default: PARSER_ASSERT_DO(
            1, "Expected either the 'OPEN_BRACKET_TOKEN' or 'LOWER_TOKEN' (<) tokens!", 
            { AST_unload(base); list_free(&generic_types); stack_pop(&ctx->scopes.stack, NULL); }
        );
    }

    ast_node_t* args = AST_create_node_bt(CREATE_SCOPE_TOKEN);
    PARSER_ASSERT_DO(
        !args, "Can't create a base for the function's arguments!", 
        { AST_unload(base); list_free(&generic_types); stack_pop(&ctx->scopes.stack, NULL); }
    );

    AST_add_node(base, args);
    args->sinfo.s_id = args_scope;

    annotations_summary_t annots = { .section = NULL, .salign = SMT_NULL, .is_entry = 0, .is_naked = 0 };
    ANNOT_read_annotations(&ctx->annots, &annots);

    symbol_id_t preserved_tid = NO_SYMBOL_ID;
    stack_top(&ctx->types, (void**)&preserved_tid);
    stack_push(&ctx->types, (void*)NO_SYMBOL_ID);

    forward_token(it, 1);
    PARSER_ASSERT_DO(
        !cpl_parse_funcdef_args(it, ctx, smt, (long)args), "Can't parse function's arguments!",
        { AST_unload(base); list_free(&generic_types); ANNOT_destroy_summary(&annots); stack_pop(&ctx->scopes.stack, NULL); }
    );

    ast_node_t* ret_type = NULL;
    if (consume_token(it, RETURN_TYPE_TOKEN)) {
        forward_token(it, 1);
        ret_type = AST_create_node(CURRENT_TOKEN);
        ret_type->sinfo.t_id = type_lookup(ret_type->t, ctx, smt);
        if (ret_type->sinfo.t_id != NO_SYMBOL_ID) ret_type->t->t_type  = EXTRACT_TYPE_TYPE(ret_type->sinfo.t_id, smt);
        else                                      ret_type->sinfo.t_id = TPTB_add_info_from_token(base->sinfo.s_id, ret_type->t, NO_SYMBOL_ID, &smt->t);
        AST_add_node(name, ret_type);
        forward_token(it, 1);
    }

    string_t* virt_name = NULL;
    if (annots.is_entry || annots.is_vname) {
        if (
            !annots.fname && 
            annots.is_entry
        ) annots.fname = create_string(CONF_get_entry_name());  
        virt_name = annots.fname;
    }

    int vargs = 0;
    int local = ctx->carry.pfunc != NO_SYMBOL_ID ? 1 : 0;
    for (ast_node_t* t = args->c; t && t->t && t->t->t_type != SCOPE_TOKEN; t = t->siblings.n) {
        if (t->t->t_type == VAR_ARGUMENTS_TOKEN) {
            vargs = 1;
            break;
        }
    }

    if (base_type) {
        token_t tmp = { .body = base_type };
        symbol_id_t base_tid = type_lookup(&tmp, ctx, smt);
        type_info_t ti;
        if (
            TPTB_get_info_id(base_tid, &ti, &smt->t)
        ) name->sinfo.s_id = ti.t == TYPE_CUSTOM ? ti.body.custom.cs_id : NO_SYMBOL_ID;
        destroy_string(base_type);
    }

    if (preserved_tid != NO_SYMBOL_ID) {
        type_info_t parent_ti; /* Mark function abstract by default, if this is an interface */
        if (TPTB_get_info_id(preserved_tid, &parent_ti, &smt->t)) {
            annots.is_abstract = parent_ti.body.custom.layout.interface;
        }
    }

    name->sinfo.v_id = FNTB_add_info(
        name->t->body, virt_name, 
        (func_info_flags_t) {
            .global   = base->t->flags.glob,            .local    = local,            .entry    = annots.is_entry, 
            .naked    = annots.is_naked ? 1 : 0,        .vargs    = vargs,            .onlybody = annots.is_onlybody,
            .generic  = list_size(&generic_types) != 0, .inln     = annots.do_inline, .self     = annots.is_self, 
            .abi      = annots.is_abi,                  .weak     = annots.is_weak,   .vname    = annots.is_vname,
            .abstract = annots.is_abstract,             .override = annots.is_override
        },
        name->sinfo.s_id, args, ret_type, &smt->f
    );
    
    if (preserved_tid != NO_SYMBOL_ID) {
        symbol_id_t type = TPTB_add_info_from_token(base->sinfo.s_id, base->t, name->sinfo.v_id, &smt->t);
        if (!TPTB_has_field(type, preserved_tid, &smt->t)) {
            type_info_t parent_ti;
            if ( /* this is an abstract / override method */
                (annots.is_override || annots.is_abstract || 
                ( /* or parent has a virtual table */
                    annots.is_self && 
                    (
                        TPTB_get_info_id(preserved_tid, &parent_ti, &smt->t) && 
                        parent_ti.t == TYPE_CUSTOM && parent_ti.body.custom.layout.vtable
                    )
                )) && /* This isn't a generic function */
                !list_size(&generic_types)
            ) {
                TPTB_enable_vtable(preserved_tid, &smt->t);                                                           /* Enable virtual table for the container       */
                if (annots.is_override || annots.is_abstract) {
                    symbol_id_t base_method = TPTB_set_as_vtable_method(preserved_tid, type, name->t->body, &smt->t); /* Link method to the container's virtual table */
                    if (base_method != SMT_NULL) {                                                                    /* Copy flags from a interface method           */
                        type_info_t base_method_ti;
                        func_info_t base_fi;
                        if (
                            TPTB_get_info_id(base_method, &base_method_ti, &smt->t) &&
                            FNTB_get_info_id(base_method_ti.body.method.f_id, &base_fi, &smt->f)
                        ) {
                            func_info_flags_t base_flags = base_fi.flags;
                            if (annots.is_override) {
                                base_flags.abstract = 0;
                                base_flags.override = 1;
                            }
                            
                            FNTB_rewrite_flags(name->sinfo.v_id, base_flags, &smt->f);
                        }
                    }
                }
            }

            TPTB_add_as_child(preserved_tid, type, name->t->body, SMT_NULL, &smt->t);
        }
    }

    if (local) FNTB_add_local(ctx->carry.pfunc, name->sinfo.v_id, &smt->f);
    else { /* Local function doesn't have a section. It copies position of its parent */
        if (annots.is_nosec)      annots.section = create_string(CONF_get_no_section());
        else if (!annots.section) annots.section = create_string(CONF_get_code_section());
        SCTB_move_to_section(annots.section, annots.salign, name->sinfo.v_id, SECTION_ELEMENT_FUNCTION, &smt->c);
    }

    ANNOT_destroy_summary(&annots);

    /* Prototype detected */
    if (CURRENT_TOKEN->t_type == DELIMITER_TOKEN) {
        if (!FNTB_has_generic_types(name->sinfo.v_id, &smt->f)) {
            foreach (symbol_id_t t_id, &generic_types) {
                FNTB_register_generic_type(name->sinfo.v_id, t_id, &smt->f);
            }
        }
        
        base->t->t_type = FUNC_PROT_TOKEN;
        stack_pop(&ctx->scopes.stack, NULL);
        stack_pop(&ctx->types, NULL);
        list_free(&generic_types);

        FNTB_update_func(name->sinfo.v_id, FNTB_ONLY_FLAGS(FNTB_SET_EXTERNAL(FNTB_SHALLOW_EXTERN)), &smt->f);
        return base;
    }

    PARSER_ASSERT_DO(
        annots.is_abstract, "Abstract function can't have a body!", 
        { AST_unload(base); list_free(&generic_types); stack_pop(&ctx->scopes.stack, NULL); stack_pop(&ctx->types, NULL); }
    );

    /* Implementation rewrites prototype's types */
    FNTB_clear_generic_types(name->sinfo.v_id, &smt->f);
    foreach (symbol_id_t t_id, &generic_types) {
        FNTB_register_generic_type(name->sinfo.v_id, t_id, &smt->f);
    }

    ast_node_t* body = NULL;
    PRESERVE_AST_CARRY_ARG({ body = cpl_parse_scope(it, ctx, smt, 1); }, name->sinfo.v_id);
    PARSER_ASSERT_DO(
        !body, "Error during the function's body parsing!", 
        { AST_unload(base); list_free(&generic_types); stack_pop(&ctx->scopes.stack, NULL); stack_pop(&ctx->types, NULL); }
    );
    AST_add_node(args, body);
    
    list_free(&generic_types);
    stack_pop(&ctx->scopes.stack, NULL);
    stack_pop(&ctx->types, NULL);

    FNTB_update_func(name->sinfo.v_id, FNTB_ONLY_FLAGS(FNTB_SET_EXTERNAL(FNTB_NO_EXTERN)), &smt->f);
    return base;
})
