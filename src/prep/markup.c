#include <prep/markup.h>

typedef struct {
    char*        value;
    token_type_t type;
} markup_token_t;

#define LEXEM(n, t) { .value = n, .type = t }
static const markup_token_t _lexems[] = {
    /* Special single place tokens. */
    LEXEM(EXTERN_COMMAND,         EXTERN_TOKEN),
    LEXEM(START_COMMAND,          START_TOKEN),
    LEXEM(EXIT_COMMAND,           EXIT_TOKEN),
    LEXEM(RETURN_TYPE_COMMAND,    RETURN_TYPE_TOKEN),
    LEXEM(ANNOTATION_COMMAND,     ANNOTATION_TOKEN),

    /* Bracket tokens. */
    LEXEM(OPEN_BLOCK,             OPEN_BLOCK_TOKEN),
    LEXEM(CLOSE_BLOCK,            CLOSE_BLOCK_TOKEN),
    LEXEM(OPEN_INDEX,             OPEN_INDEX_TOKEN),
    LEXEM(CLOSE_INDEX,            CLOSE_INDEX_TOKEN),
    LEXEM(OPEN_BRACKET,           OPEN_BRACKET_TOKEN),
    LEXEM(CLOSE_BRACKET,          CLOSE_BRACKET_TOKEN),

    LEXEM(CONTAINER_COMMAND,      CONTAINER_TOKEN),
    LEXEM(INTERFACE_COMMAND,      INTERFACE_TOKEN),
    LEXEM(DOT_COMMAND,            DOT_TOKEN),
    LEXEM(STAT_COMMAND,           STAT_TOKEN),

    /* Function and jmp tokens. */
    LEXEM(FUNCTION_COMMAND,       FUNC_TOKEN),
    LEXEM(SIGNATURE_COMMAND,      SIGNATURE_TOKEN),
    LEXEM(RETURN_COMMAND,         RETURN_TOKEN),
    LEXEM(SYSCALL_COMMAND,        SYSCALL_TOKEN),
    LEXEM(ASM_COMMAND,            ASM_TOKEN),
    LEXEM(VAR_ARGUMENTS_COMMAND,  VAR_ARGUMENTS_TOKEN),
    LEXEM(LAMBDA_COMMAND,         LAMBDA_TOKEN),

    /* Variable modifiers */
    LEXEM(DREF_COMMAND,           DREF_TYPE_TOKEN),
    LEXEM(REF_COMMAND,            REF_TYPE_TOKEN),
    LEXEM(PTR_COMMAND,            PTR_TYPE_TOKEN),
    LEXEM(RO_COMMAND,             RO_TYPE_TOKEN),
    LEXEM(GLOB_COMMAND,           GLOB_TYPE_TOKEN),
    LEXEM(NEGATIVE_COMMAND,       NEGATIVE_TOKEN),
    LEXEM(NOT_COMMAND,            NOT_TOKEN),

    /* Variable tokens. */
    LEXEM(I0_VARIABLE,            I0_TYPE_TOKEN),
    LEXEM(F64_VARIABLE,           F64_TYPE_TOKEN),
    LEXEM(F32_VARIABLE,           F32_TYPE_TOKEN),
    LEXEM(I64_VARIABLE,           I64_TYPE_TOKEN),
    LEXEM(I32_VARIABLE,           I32_TYPE_TOKEN),
    LEXEM(I16_VARIABLE,           I16_TYPE_TOKEN),
    LEXEM(I8_VARIABLE,            I8_TYPE_TOKEN),
    LEXEM(U64_VARIABLE,           U64_TYPE_TOKEN),
    LEXEM(U32_VARIABLE,           U32_TYPE_TOKEN),
    LEXEM(U16_VARIABLE,           U16_TYPE_TOKEN),
    LEXEM(U8_VARIABLE,            U8_TYPE_TOKEN),
    LEXEM(ARR_VARIABLE,           ARRAY_TYPE_TOKEN),

    /* Cast token */
    LEXEM(CONVERT_COMMAND,        CONVERT_TOKEN),

    /* Little jump tokens. */
    LEXEM(SWITCH_COMMAND,         SWITCH_TOKEN),
    LEXEM(CASE_COMMAND,           CASE_TOKEN),
    LEXEM(DEFAULT_COMMAND,        DEFAULT_TOKEN),
    LEXEM(WHILE_COMAND,           WHILE_TOKEN),
    LEXEM(LOOP_COMMAND,           LOOP_TOKEN),
    LEXEM(BREAK_COMMAND,          BREAK_TOKEN),
    LEXEM(IF_COMMAND,             IF_TOKEN),
    LEXEM(ELSE_COMMAND,           ELSE_TOKEN),
    LEXEM(SIZEOF_COMMAND,         SIZEOF_TOKEN),

    /* Binary operands. */
    LEXEM(ADDASSIGN_STATEMENT,    ADDASSIGN_TOKEN),
    LEXEM(SUBASSIGN_STATEMENT,    SUBASSIGN_TOKEN),
    LEXEM(MULASSIGN_STATEMENT,    MULASSIGN_TOKEN),
    LEXEM(DIVASSIGN_STATEMENT,    DIVASSIGN_TOKEN),
    LEXEM(MODULOASSIGN_STATEMENT, MODULOASSIGN_TOKEN),
    LEXEM(BITANDASSIGN_STATEMENT, BITANDASSIGN_TOKEN),
    LEXEM(BITORASSIGN_STATEMENT,  BITORASSIGN_TOKEN),
    LEXEM(BITXORASSIGN_STATEMENT, BITXORASSIGN_TOKEN),
    LEXEM(ASSIGN_STATEMENT,       ASSIGN_TOKEN),
    LEXEM(COMPARE_STATEMENT,      COMPARE_TOKEN),
    LEXEM(NCOMPARE_STATEMENT,     NCOMPARE_TOKEN),
    LEXEM(PLUS_STATEMENT,         PLUS_TOKEN),
    LEXEM(MINUS_STATEMENT,        MINUS_TOKEN),
    LEXEM(LARGER_STATEMENT,       LARGER_TOKEN),
    LEXEM(LARGEREQ_STATEMENT,     LARGEREQ_TOKEN),
    LEXEM(LOWER_STATEMENT,        LOWER_TOKEN),
    LEXEM(LOWEREQ_STATEMENT,      LOWEREQ_TOKEN),
    LEXEM(MULTIPLY_STATEMENT,     MULTIPLY_TOKEN),
    LEXEM(DIVIDE_STATEMENT,       DIVIDE_TOKEN),
    LEXEM(MODULO_STATEMENT,       MODULO_TOKEN),
    LEXEM(BITMOVE_LEFT_STATEMENT, BITMOVE_LEFT_TOKEN),
    LEXEM(BITMOVE_RIGHT_STATMENT, BITMOVE_RIGHT_TOKEN),
    LEXEM(BITAND_STATEMENT,       BITAND_TOKEN),
    LEXEM(BITOR_STATEMENT,        BITOR_TOKEN),
    LEXEM(BITXOR_STATEMENT,       BITXOR_TOKEN),
    LEXEM(AND_STATEMENT,          AND_TOKEN),
    LEXEM(OR_STATEMENT,           OR_TOKEN),

    /* Debug */
    LEXEM(BREAKPOINT_COMMAND,     BREAKPOINT_TOKEN),
};

/* Convert the lexem list to the lexem map.
Params:
    - `m` - Output map.

Returns 1 if succeeds. */
static int _build_lexems_map(map_t* m) {
    for (int i = 0; i < (int)(sizeof(_lexems) / sizeof(_lexems[0])); i++) {
        map_put(m, crc64((unsigned char*)_lexems[i].value, str_strlen(_lexems[i].value), 0), (void*)_lexems[i].type);
    }

    return 1;
}

int MRKP_mnemonics(list_t* tkn) {
    map_t lexems;
    map_init(&lexems, MAP_NO_CMP);
    _build_lexems_map(&lexems);
    foreach (token_t* curr, tkn) {
        long t;
        if (curr->t_type == STRING_VALUE_TOKEN || !curr->body) continue;
        if (map_get(&lexems, crc64((unsigned char*)curr->body->body, curr->body->size, 0), (void**)&t)) {
            curr->t_type = t;
        }
    }

    return map_free(&lexems);
}

/* Remove token from a list and unload it.
Params:
    - `tkns` - Target list.
    - `tkn` - Targe token. */
static inline void _remove_token(list_t* tkns, token_t* tkn) {
    list_remove(tkns, tkn);
    TKN_unload_token(tkn);
}

/* Apply modifiers in a token list.
How it works:
```cpl
    glob ro ptr i32 a;
```

Will consume the 'glob', the 'ro' and the 'ptr' token, accamulate
information and apply it on the 'i32' token. 
Params:
    - `tkn` - Token list.

Returns 1 if succeeds. */
static int _apply_modifiers(list_t* tkn) {
    basic_object_info_t cflags = { 0 };

    list_iter_t it;
    list_iter_hinit(tkn, &it);
    token_t* curr;
    while (list_iter_next(&it, (void**)&curr)) {
        token_t* next = (token_t*)list_iter_current(&it);
        if (next) switch (curr->t_type) {
            case EXTERN_TOKEN:    cflags.ext = 1;                            break;
            case GLOB_TYPE_TOKEN: cflags.glob = 1; _remove_token(tkn, curr); break;
            case PTR_TYPE_TOKEN:  cflags.ptr++;    _remove_token(tkn, curr); break;
            case RO_TYPE_TOKEN:   cflags.ro   = 1; _remove_token(tkn, curr); break;
            default: {
                str_memcpy(&curr->flags, &cflags, sizeof(basic_object_info_t));
                str_memset(&cflags, 0, sizeof(basic_object_info_t));
                break;
            }
        }
    }

    return 1;
}

int MRKP_variables(list_t* tkn) {
    return _apply_modifiers(tkn);
}
