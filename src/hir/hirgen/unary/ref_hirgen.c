#include <hir/hirgens/hirgens.h>

hir_subject_t* HIR_reference_subject(hir_subject_t* src, sym_table_t* smt, int increment) {
    /* We need to dereference the type of an element, 
       if this is an array type. */
    hir_subject_type_t src_type = src->t;
    int src_ptr = src->ptr;

    if (HIR_is_arrtype(src_type)) {
        array_info_t ai;
        if (ARTB_get_info(src->storage.var.v_id, &ai, &smt->a)) {
            token_t tmp = { .t_type = ai.elements_info.el_type };
            src_type = HIR_get_tmptype_tkn(&tmp, 0);
            src_ptr = ai.elements_info.el_flags.ptr;
        }
    }

    /* If the source variable was a custom typed variable, we mst
       preserve its type thru reference operation */
    symbol_id_t ref_id = VRTB_add_info(NULL, HIR_get_tmptkn_type(src_type), NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v);
    variable_info_t src_info;
    if (
        HIR_is_vartype(src->t)                                      &&
        VRTB_get_info_id(src->storage.var.v_id, &src_info, &smt->v) &&
        src_info.t_id != NO_SYMBOL_ID
    ) VRTB_update_type(ref_id, FIELD_NO_CHANGE, src_info.t_id, &smt->v);

    hir_subject_t* ref = HIR_SUBJ_TMPVAR(src_type, ref_id);
    if (increment) ref->ptr = MAX(src_ptr + 1, 0);
    else           ref->ptr = src_ptr;
    return ref;
}

hir_subject_t* HIR_generate_ref(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    HIR_SET_CURRENT_POS(ctx, node);
    if (
        node->c && node->c->t && 
        node->c->t->t_type == MEMBER_ACCESS_TOKEN
    ) {
        type_info_t ti = { 0 };
        hir_subject_t* head = HIR_point_to_field(node->c, ctx, &ti, smt);
        array_info_t ai;
        variable_info_t vi;
        if (
            ti.t == TYPE_ARRAY &&
            HIR_find_member_variable(&ti, &vi, smt) &&
            ARTB_get_info(vi.v_id, &ai, &smt->a)
        ) return HIR_load_array_field_head(head, &ai, ctx, smt);
        return head;
    }
    if (
        node->c && node->c->t &&
        node->c->t->t_type == INDEXATION_TOKEN
    ) return HIR_generate_ref_indexation(node->c, ctx, smt);
    hir_subject_t* src = HIR_generate_elem(node->c, ctx, smt);
    hir_subject_t* ref = HIR_reference_subject(src, smt, 1);
    HIR_BLOCK2(ctx, HIR_REF, ref, src);
    return ref;
}
