#include <ast/astgen/astgen.h>

typedef struct {
    ast_node_t*         (*handler)(PARSER_ARGS);
    const token_type_t* types;
    int                 types_count;
    long                dcarry;
} handler_t;

/* Handler for token type parsing */
#define HANDLER(func, carry, ...)                                                                \
    {                                                                                            \
        .handler = func,                                                                         \
        .dcarry  = carry,                                                                        \
        .types = (const token_type_t[]){ __VA_ARGS__ },                                          \
        .types_count = (int)sizeof((const token_type_t[]){ __VA_ARGS__ }) / sizeof(token_type_t) \
    }

/* Token handlers. The main point where the parser decides which parser
must handle the provided token.
Note: ! If you're extending the parser, add a new handler here ! */
static const handler_t handlers[] = {
    HANDLER(cpl_parse_contdef,           0, CONTAINER_TOKEN, INTERFACE_TOKEN),
    HANDLER(cpl_parse_annot,             0, ANNOTATION_TOKEN),
    HANDLER(cpl_parse_start,             0, START_TOKEN),
    HANDLER(cpl_parse_asm,               0, ASM_TOKEN),
    HANDLER(cpl_parse_scope,             1, OPEN_BLOCK_TOKEN),
    HANDLER(cpl_parse_switch,            0, SWITCH_TOKEN),
    HANDLER(cpl_parse_if,                0, IF_TOKEN),
    HANDLER(cpl_parse_while,             0, WHILE_TOKEN),
    HANDLER(cpl_parse_loop,              0, LOOP_TOKEN),
    HANDLER(cpl_parse_break,             0, BREAK_TOKEN),
    HANDLER(cpl_parse_syscall,           0, SYSCALL_TOKEN),
    HANDLER(cpl_parse_breakpoint,        0, BREAKPOINT_TOKEN),
    HANDLER(cpl_parse_extern,            0, EXTERN_TOKEN),
    HANDLER(cpl_parse_function,          0, FUNC_TOKEN),
    HANDLER(cpl_parse_return,            0, EXIT_TOKEN, RETURN_TOKEN),
    HANDLER(cpl_parse_array_declaration, 0, ARRAY_TYPE_TOKEN),
    HANDLER(
        cpl_parse_variable_declaration, NO_SYMBOL_ID,
        F32_TYPE_TOKEN, F64_TYPE_TOKEN,
        I8_TYPE_TOKEN,  I16_TYPE_TOKEN, I32_TYPE_TOKEN, I64_TYPE_TOKEN,
        U8_TYPE_TOKEN,  U16_TYPE_TOKEN, U32_TYPE_TOKEN, U64_TYPE_TOKEN,
        I0_TYPE_TOKEN,  SIGNATURE_TOKEN
    ),
    HANDLER(
        cpl_parse_expression, 0,
        VARIABLE_TOKEN,
        NEGATIVE_TOKEN,       NOT_TOKEN,
        REF_TYPE_TOKEN,       DREF_TYPE_TOKEN,
        ARR_VARIABLE_TOKEN,
        I0_VARIABLE_TOKEN,    I8_VARIABLE_TOKEN,           I16_VARIABLE_TOKEN, I32_VARIABLE_TOKEN, I64_VARIABLE_TOKEN,
        F32_VARIABLE_TOKEN,   F64_VARIABLE_TOKEN,
        U8_VARIABLE_TOKEN,    U16_VARIABLE_TOKEN,          U32_VARIABLE_TOKEN, U64_VARIABLE_TOKEN,
        OPEN_BRACKET_TOKEN,
        UNKNOWN_STRING_TOKEN, UNKNOWN_FLOAT_NUMERIC_TOKEN, UNKNOWN_NUMERIC_TOKEN
    ),
};
#undef HANDLER

/* Try to parse declarations that depend on symbols known only at parse time.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.

Returns an AST node if token starts a dynamic declaration. Otherwise returns NULL. */
static ast_node_t* _dynamic_navigation_handler(PARSER_ARGS) {
    PARSER_ARGS_USE;
    symbol_id_t type = type_lookup(CURRENT_TOKEN, ctx, smt);
    if (
        CURRENT_TOKEN->t_type != ARRAY_TYPE_TOKEN                          &&
        (look_next_token(it) && look_next_token(it)->t_type != STAT_TOKEN) &&
        (TKN_is_builtin_type(CURRENT_TOKEN) || type != NO_SYMBOL_ID)
    ) return cpl_parse_variable_declaration(it, ctx, smt, type);
    return NULL;
}

/* Try to parse current token using the static token-to-parser handler table.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.

Returns an AST node if token was handled. Otherwise returns NULL. */
static ast_node_t* _static_navigation_handler(PARSER_ARGS) {
    PARSER_ARGS_USE;
    for (int i = 0; i < (int)(sizeof(handlers) / sizeof(handlers[0])); i++) {
        for (int j = 0; j < handlers[i].types_count; j++) {
            if (handlers[i].types[j] == CURRENT_TOKEN->t_type) {
                return handlers[i].handler(it, ctx, smt, handlers[i].dcarry);
            }
        }
    }

    return NULL;
}

/* Parsers collection navigation.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.

Returns an AST node. */
static ast_node_t* _navigation_handler(PARSER_ARGS) {
    ast_node_t* dyn = _dynamic_navigation_handler(it, ctx, smt, carry);
    if (dyn) return dyn;
    return _static_navigation_handler(it, ctx, smt, carry);
}

/* Parse one element from the current parser position.
Params:
    - `it` - Current iterator.
    - `ctx` - AST context.
    - `smt` - Symtable.

Returns an AST node. */
DEFINE_PARSER(cpl_parse_element, {
    return _navigation_handler(it, ctx, smt, carry);
})

DEFINE_PARSER(cpl_parse_block, {
    ast_node_t* base = AST_create_node_bt(CREATE_SCOPE_TOKEN);
    PARSER_ASSERT(!base, NULL, "Can't create a basic block for the scope block!");

    stack_top(&ctx->scopes.stack, (void**)&base->sinfo.s_id);
    while (CURRENT_TOKEN && CURRENT_TOKEN->t_type != carry) {
        ast_node_t* block = cpl_parse_element(it, ctx, smt, carry);
        PARSER_ASSERT_DO(
            CONF_is_parser_error(), "There is a critical error during the block parsing!", 
            { AST_unload(block); AST_unload(base); }
        );
        
        if (block) AST_add_node(base, block);  /* If parsing succeeds, add the parsed node to the body */
        else if (!forward_token(it, 1)) break; /* If there is an error, advance to the next token      */
    }

    forward_token(it, 1); /* Move past the block terminator */
    return base;
})
