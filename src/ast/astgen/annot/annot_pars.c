#include <ast/astgen/astgen.h>

static int _extract_params_from_brackets(list_iter_t* it, token_t** first, token_t** second) {
    if (!consume_token(it, OPEN_BRACKET_TOKEN)) return 1;
    forward_token(it, 1);
    while (CURRENT_TOKEN && CURRENT_TOKEN->t_type != CLOSE_BRACKET_TOKEN) {
        if (CURRENT_TOKEN->t_type == COMMA_TOKEN) {
            forward_token(it, 1);
            continue;
        }

        if (!*first)       *first  = CURRENT_TOKEN;
        else if (!*second) *second = CURRENT_TOKEN;
        forward_token(it, 1);
    }

    if (!CURRENT_TOKEN) return 0;
    consume_token(it, CLOSE_BRACKET_TOKEN);
    return 1;
}

static void _pack_param(token_t* tkn, ast_ctx_t* ctx, sym_table_t* smt, int allow_variable, annotation_param_t* box) {
    str_memset(box, 0, sizeof(annotation_param_t));
    box->value = SMT_NULL;
    box->t     = ANNOTATION_VALUE_PARAM;
    if (!tkn) return;

    box->filled = 1;
    if (allow_variable) {
        token_t* lookup_tkn = TKN_copy_token(tkn);
        ast_node_t tmp;
        str_memset(&tmp, 0, sizeof(ast_node_t));

        tmp.t          = lookup_tkn;
        tmp.sinfo.s_id = NO_SYMBOL_ID;
        tmp.sinfo.v_id = NO_SYMBOL_ID;
        tmp.sinfo.t_id = NO_SYMBOL_ID;

        if (var_lookup(&tmp, ctx, smt) && TKN_is_variable(lookup_tkn) && tmp.sinfo.v_id != NO_SYMBOL_ID) {
            box->t    = ANNOTATION_VARIABLE_PARAM;
            box->v_id = tmp.sinfo.v_id;
            TKN_unload_token(lookup_tkn);
            return;
        }

        TKN_unload_token(lookup_tkn);
    }

    box->string = tkn->body;
    box->value  = tkn->body ? tkn->body->to_llong(tkn->body) : SMT_NULL;
}

#define ADD_ANNOTATION_HANDLER(n, t)               \
    if (raw_annot->requals(raw_annot, n)) {        \
        return ANNOT_create_annotation(t, &a, &b); \
    }
static annotation_t* _parse_annotation_content(list_iter_t* it, ast_ctx_t* ctx, sym_table_t* smt) {
    token_t *fp = NULL, *sp = NULL;
    string_t* raw_annot = CURRENT_TOKEN->body;
    _extract_params_from_brackets(it, &fp, &sp);

    annotation_param_t a, b;
    int allow_variable = raw_annot->requals(raw_annot, COUNT_ANNOTATION_COMMAND);
    _pack_param(fp, ctx, smt, allow_variable, &a);
    _pack_param(sp, ctx, smt, allow_variable, &b);
    
    ADD_ANNOTATION_HANDLER(SECTN_ANNOTATION_COMMAND, SECTION_ANNOTATION);
    ADD_ANNOTATION_HANDLER(NOSEC_ANNOTATION_COMMAND, NOSECTION_ANNOTATION);
    ADD_ANNOTATION_HANDLER(ALIGN_ANNOTATION_COMMAND, ALIGN_ANNOTATION);
    ADD_ANNOTATION_HANDLER(NAKED_ANNOTATION_COMMAND, NAKED_ANNOTATION);
    ADD_ANNOTATION_HANDLER(ENTRY_ANNOTATION_COMMAND, ENTRY_ANNOTATION);
    ADD_ANNOTATION_HANDLER(NOFAL_ANNOTATION_COMMAND, NOFALL_ANNOTATION);
    ADD_ANNOTATION_HANDLER(NTLAZ_ANNOTATION_COMMAND, NOTLAZY_ANNOTATION);
    ADD_ANNOTATION_HANDLER(STRGH_ANNOTATION_COMMAND, STRAIGHT_ANNOTATION);
    ADD_ANNOTATION_HANDLER(COUNT_ANNOTATION_COMMAND, COUNTER_ANNOTATION);
    ADD_ANNOTATION_HANDLER(HOTSC_ANNOTATION_COMMAND, HOT_ANNOTATION);
    ADD_ANNOTATION_HANDLER(COLDS_ANNOTATION_COMMAND, COLD_ANNOTATION);
    ADD_ANNOTATION_HANDLER(REGST_ANNOTATION_COMMAND, REGISTER_ANNOTATION);
    ADD_ANNOTATION_HANDLER(POPRG_ANNOTATION_COMMAND, POPARG_ANNOTATION);
    ADD_ANNOTATION_HANDLER(PPREG_ANNOTATION_COOMAND, POPREG_ANNOTATION);
    ADD_ANNOTATION_HANDLER(INLNE_ANNOTATION_COMMAND, INLINE_ANNOTATION);
    ADD_ANNOTATION_HANDLER(SSELF_ANNOTATION_COMMAND, SELF_ANNOTATION);
    ADD_ANNOTATION_HANDLER(LIKEC_ANNOTATION_COMMAND, LIKEC_ANNOTATION);
    ADD_ANNOTATION_HANDLER(UNION_ANNOTATION_COMMAND, UNION_ANNOTATION);
    ADD_ANNOTATION_HANDLER(WEAKS_ANNOTATION_COMMAND, WEAK_ANNOTATION);
    ADD_ANNOTATION_HANDLER(ABICC_ANNOTATION_COMMAND, ABI_ANNOTATION);
    ADD_ANNOTATION_HANDLER(BODYO_ANNOTATION_COMMAND, ONLYBODY_ANNOTATION);
    ADD_ANNOTATION_HANDLER(VNAME_ANNOTATION_COMMAND, VNAME_ANNOTATION);
    ADD_ANNOTATION_HANDLER(NNULL_ANNOTATION_COMMAND, NOTNULL_ANNOTATION);
    ADD_ANNOTATION_HANDLER(VOLAT_ANNOTATION_COMMAND, VOLATILE_ANNOTATION);
    ADD_ANNOTATION_HANDLER(VTABL_ANNOTATION_COMMAND, VTABLE_ANNOTATION);
    ADD_ANNOTATION_HANDLER(ABSTR_ANNOTATION_COMMAND, ABSTRACT_ANNOTATION);
    ADD_ANNOTATION_HANDLER(OVERD_ANNOTATION_COMMAND, OVERRIDE_ANNOTAITON);
    return ANNOT_create_annotation(UNKNOWN_ANNOTATION, NULL, NULL);
}
#undef ADD_ANNOTATION_HANDLER

DEFINE_PARSER(cpl_parse_annot, {
    PARSER_ASSERT(!consume_token(it, OPEN_INDEX_TOKEN), NULL, "'@' should be followed by '['!");
    PARSER_ASSERT(!consume_token(it, UNKNOWN_STRING_TOKEN), NULL, "Expected a string token after the annotation's start!");
    annotation_t* annot = _parse_annotation_content(it, ctx, smt);
    PARSER_ASSERT(!annot, NULL, "Annotation parse error!");
    stack_push(&ctx->annots, annot);
    return NULL;
})
