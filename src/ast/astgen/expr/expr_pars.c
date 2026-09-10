#include <ast/astgen/astgen.h>

/* Parse left part of a stmt.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.
    - `na` - No assign.
             Note: By default (0), this function parses an entire 
                   expression with a assign symbol. That means, that
                   expressions such as `a = b`, `a + b = a + b` will
                   be full parsed.
                   If you want to parse only the left part (before assign),
                   set this flag to 1.

Returns an AST node. */
static ast_node_t* _parse_primary(list_iter_t*, ast_ctx_t*, sym_table_t*, int);

/* Parse expression that looks like: <stmt> <op> <stmt>. 
Note: <stmt> here can be either a simple <(a..> or a complex sub-stmt.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.
    - `mp` - Minimal priority. 
             Note: Defenition of a minimal priority of a token
                   that will stop parsing for the current level.
    - `na` - No assign.
             Note: By default (0), this function parses an entire 
                   expression with a assign symbol. That means, that
                   expressions such as `a = b`, `a + b = a + b` will
                   be full parsed.
                   If you want to parse only the left part (before assign),
                   set this flag to 1.
                   If you want to parse only the primary - set this flag to 2.

Returns an AST node. */
static ast_node_t* _parse_binary_expression(list_iter_t* it, ast_ctx_t* ctx, sym_table_t* smt, int mp, int na) {
    SAVE_TOKEN_POINT;
    ast_node_t* left = _parse_primary(it, ctx, smt, na);
    if (!left) {
        RESTORE_TOKEN_POINT;
        return NULL;
    }

    while (CURRENT_TOKEN) {
        switch (CURRENT_TOKEN->t_type) {
            /* Generic type resolution (possible) */
            case LOWER_TOKEN: {
                if (left->t->t_type != CALL_ADDR_TOKEN) goto _default_operator;
                symbol_id_t type = type_lookup(look_next_token(it), ctx, smt);
                if (type == NO_SYMBOL_ID && !TKN_is_builtin_type(look_next_token(it))) goto _default_operator;
                forward_token(it, 1);
                do {
                    ast_node_t* type_node = AST_create_node(CURRENT_TOKEN);
                    type = type_lookup(type_node->t, ctx, smt);
                    PARSER_ASSERT(!type_node, left, "Error during a generic type operation parsing!");
                    AST_add_node(left, type_node);
                    if (type != NO_SYMBOL_ID) {
                        type_node->sinfo.t_id = type;
                        type_node->t->t_type  = EXTRACT_TYPE_TYPE(type, smt);
                    }

                    if (consume_token(it, COMMA_TOKEN)) forward_token(it, 1);
                } while (CURRENT_TOKEN->t_type != LARGER_TOKEN);
                forward_token(it, 1);
                break;
            }
            /* Member access */
            case STAT_TOKEN: left->sinfo.t_id = type_lookup(left->t, ctx, smt); 
                             __attribute__ ((fallthrough));
            case DOT_TOKEN: {
                forward_token(it, 1);
                symbol_id_t field_type = TPTB_resolve_child(left->sinfo.t_id, CURRENT_TOKEN->body, &smt->t);
                PARSER_ASSERT(
                    left->sinfo.t_id == NO_SYMBOL_ID || field_type == NO_SYMBOL_ID, left, 
                    "Unknown container or a container's field!"
                );

                ast_node_t* member = AST_create_node(CURRENT_TOKEN);
                PARSER_ASSERT(!member, left, "Can't create a member!");

                type_info_t c_ti;
                TPTB_get_info_id(field_type, &c_ti, &smt->t);
                /* If this type is a method, we must stop going, remember
                   existed chain in the 'left' as a self pointer. */
                if (c_ti.t == TYPE_METHOD) {
                    member->sinfo.v_id = c_ti.body.method.f_id;
                    member->t->t_type  = CALL_ADDR_TOKEN;

                    func_info_t fi;
                    if (
                        FNTB_get_info_id(member->sinfo.v_id, &fi, &smt->f) && 
                        fi.flags.self
                    ) member->self = left;
                    else AST_unload(left);
                    
                    left = member;
                    forward_token(it, 1);
                    break;
                }

                ast_node_t* base = AST_create_node_bt(CREATE_ACCESS_TOKEN);
                if (!base) {
                    AST_unload(left);
                    RESTORE_TOKEN_POINT;
                    return NULL;
                }

                base->sinfo.t_id   = field_type;
                member->sinfo.t_id = field_type;

                AST_add_node(base, left);
                AST_add_node(base, member);

                left = base;
                forward_token(it, 1);
                break;
            }
            /* Postfix tokens that are change placment in an AST tree.
               '[]' / '()' / 'as' takes two children: the pointer and the data. */
            case CONVERT_TOKEN:
            case OPEN_INDEX_TOKEN:
            case OPEN_BRACKET_TOKEN: {
                int annot_off = annotation_reserve(ctx);
                ast_node_t *target = NULL, *data = NULL;
                switch (CURRENT_TOKEN->t_type) {
                    case CONVERT_TOKEN: target = cpl_parse_conv(it, ctx, smt, 0); break;
                    case OPEN_INDEX_TOKEN: {
                        forward_token(it, 1);
                        target = AST_create_node_bt(CREATE_INDEX_TOKEN);
                        data   = cpl_parse_expression(it, ctx, smt, 1);
                        break;
                    }
                    case OPEN_BRACKET_TOKEN: {
                        forward_token(it, 1);
                        /* If it was a function addr - convert to a classic funccall */
                        if (left->t->t_type == CALL_ADDR_TOKEN) {
                            left->t->t_type = FUNC_NAME_TOKEN;
                        }

                        target = AST_create_node_bt(CREATE_CALL_TOKEN);
                        data   = cpl_parse_call_arguments(it, ctx, smt, 0);
                        if (left->self) {
#define WRAP_REFERENCE_NODE(nd) do {                                                          \
        ast_node_t* __pp = AST_create_node_bt(TKN_create_token(REF_TYPE_TOKEN, "ref", NULL)); \
        AST_add_node(__pp, nd);                                                               \
        nd = __pp;                                                                            \
    } while (0)
                            target->self = left->self;
                            type_info_t self_ti;
                            TPTB_get_info_id(left->self->sinfo.t_id, &self_ti, &smt->t);
                            variable_info_t self_vi;
                            if (
                                (
                                    VRTB_find_by_type_id(self_ti.id, &self_vi, &smt->v) && !self_vi.vfs.ptr && 
                                    self_ti.member.p != NO_SYMBOL_ID // TODO: This is redundant code. It was written back when type was a single entity. Now this is an abstraction which represents a big part of logic. 
                                ) ||                                       // It's essential to get rid from this logic and move it to the smt as a separated structure which tracks these kinds of link
                                (
                                    !left->self->t->flags.ptr                 && /* If self doesn't referenced                       */
                                    left->self->t->t_type != INDEXATION_TOKEN && /* Any indexation operation already have referenced */
                                    self_ti.member.p == NO_SYMBOL_ID             /* And this isn't a field in a container            */
                                )
                            ) WRAP_REFERENCE_NODE(left->self);
                            AST_insert_node(data, left->self);
                            left->self = NULL;
                        }
#undef WRAP_REFERENCE_NODE
                        break;
                    }
                    default: break;
                }

                PARSER_ASSERT_DO(
                    !target, "Error during a postfix operation parsing!", 
                    { AST_unload(left); AST_unload(target); AST_unload(data); }
                );
                
                ast_node_t* tmp = left;
                left = target;
                if (tmp) AST_add_node(left, tmp);
                if (
                    tmp && left->t->t_type == INDEXATION_TOKEN
                ) left->sinfo.t_id = TPTB_get_indexed_type(tmp->sinfo.t_id, &smt->t);
                if (data) {
                    AST_add_node(left, data);
                    forward_token(it, 1);
                }

                annotation_unreserve(ctx, annot_off);
                break;
            }
            /* Default operators such as:
               plus, minus, multiply, etc. */
            default: {
_default_operator: {}
                if (na == 2) goto _stop_expression_parsing;
                int p = TKN_token_priority(CURRENT_TOKEN);
                if (
                    p < mp  ||                                         /* Stop at a token with a lower priority   */
                    p == -1 ||                                         /* Stop at an unknown priority             */
                    (na == 1 && CURRENT_TOKEN->t_type == ASSIGN_TOKEN) /* Stop at an assign if there is a na == 1 */
                ) goto _stop_expression_parsing;

                int next_mp = p + 1;
                if (TKN_is_update_operator(CURRENT_TOKEN)) {
                    next_mp = p;
                }

                ast_node_t* op_node = AST_create_node(CURRENT_TOKEN);
                PARSER_ASSERT(!op_node, left, "Can't create the expression's base!");

                forward_token(it, 1);
                int annot_off = annotation_reserve(ctx);
                ast_node_t* right = _parse_binary_expression(it, ctx, smt, next_mp, na);
                PARSER_ASSERT_DO(
                    !right, "Error during the right part parse!", 
                    { AST_unload(op_node); AST_unload(left); }
                );

                annotation_unreserve(ctx, annot_off);
                DUMP_ANNOTATION_TO_NODE(ctx, left);
                AST_add_node(op_node, left);
                AST_add_node(op_node, right);
                left = op_node;
                break;
            }
        }
    }

_stop_expression_parsing: {}
    DUMP_ANNOTATION_TO_NODE(ctx, left);
    return left;
}
#undef WRAP_REFERENCE_NODE

static ast_node_t* _parse_primary(list_iter_t* it, ast_ctx_t* ctx, sym_table_t* smt, int na) {
    SAVE_TOKEN_POINT;
    if (TKN_is_close(CURRENT_TOKEN)) {
        PARSE_ERROR("Expected a token, but got a terminator!");
        return NULL;
    }
    
    while (1) {
        switch (CURRENT_TOKEN->t_type) {
            case ANNOTATION_TOKEN: {
                cpl_parse_annot(it, ctx, smt, 0);
                forward_token(it, 1);
                continue;
            }
            case OPEN_BRACKET_TOKEN: {
                forward_token(it, 1);
                int annot_off = annotation_reserve(ctx);

                symbol_id_t type = type_lookup(CURRENT_TOKEN, ctx, smt);
                ast_node_t* node = NULL;
                if (
                    (
                        (type != NO_SYMBOL_ID || TKN_is_builtin_type(CURRENT_TOKEN)) &&
                        look_next_token(it) && look_next_token(it)->t_type == UNKNOWN_STRING_TOKEN
                    ) ||                                                           /* We found a type         */
                    CURRENT_TOKEN->t_type == CLOSE_BRACKET_TOKEN                   /* We found a closed token */
                ) node = cpl_parse_lambda(it, ctx, smt, 0);
                else {
                    node = _parse_binary_expression(it, ctx, smt, 0, na == 2 ? 1 : na);
                    forward_token(it, 1);
                }
                if (!node) {
                    PARSE_ERROR("Error during a bracket expression parsing!");
                    RESTORE_TOKEN_POINT;
                    return NULL;
                }

                annotation_unreserve(ctx, annot_off);
                return node;
            }
            case SIZEOF_TOKEN:    return cpl_parse_sizeof(it, ctx, smt, 0);
            case SYSCALL_TOKEN:   return cpl_parse_syscall(it, ctx, smt, 0);
            case NOT_TOKEN:
            case NEGATIVE_TOKEN:
            case REF_TYPE_TOKEN:
            case DREF_TYPE_TOKEN: return cpl_parse_unary(it, ctx, smt, 0);
            default: goto _primary_resolve_complete;
        }
    }
_primary_resolve_complete: {}

    ast_node_t* node = AST_create_node(CURRENT_TOKEN);
    PARSER_ASSERT(!node, NULL, "Can't create a base for a value!");

    switch (node->t->t_type) {
        case STRING_VALUE_TOKEN: {
            node->sinfo.v_id  = STTB_add_info(node->t->body, STR_INDEPENDENT, &smt->s);
            string_t* section = create_string(CONF_get_ro_section());
            SCTB_move_to_section(section, SMT_NULL, node->sinfo.v_id, SECTION_ELEMENT_STRING, &smt->c);
            destroy_string(section);
            break;
        }
        default: break;
    }

    var_lookup(node, ctx, smt);
    forward_token(it, 1);
    return node;
}

DEFINE_PARSER(cpl_parse_expression, {
    return _parse_binary_expression(it, ctx, smt, 0, carry);
})
