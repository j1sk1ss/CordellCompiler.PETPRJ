#include <hir/hirgens/hirgens.h>

static inline ast_node_t* _member_owner_node(ast_node_t* node) {
    return node ? node->c : NULL;
}

static inline ast_node_t* _member_name_node(ast_node_t* node) {
    ast_node_t* owner = _member_owner_node(node);
    return owner ? owner->siblings.n : NULL;
}

static inline symbol_id_t _member_owner_id(ast_node_t* node) {
    ast_node_t* owner = _member_owner_node(node);
    return owner ? owner->sinfo.t_id : NO_SYMBOL_ID;
}

static inline string_t* _member_name(ast_node_t* node) {
    ast_node_t* name = _member_name_node(node);
    return name && name->t ? name->t->body : NULL;
}

int HIR_find_member_variable(type_info_t* field_info, symbol_id_t owner_id, string_t* name, variable_info_t* var_info, sym_table_t* smt) {
    if (!field_info) return 0;

    member_info_t member_info;
    if (!TPTB_get_member_info(owner_id, field_info->id, name, &member_info, &smt->t)) {
        return VRTB_find_by_type_id(field_info->id, var_info, &smt->v);
    }

    type_info_t owner_info;
    owner_id = TPTB_resolve_parent(member_info.parent, &smt->t);
    if (
        !TPTB_get_info_id(owner_id, &owner_info, &smt->t) ||
        owner_info.t != TYPE_CUSTOM
    ) return 0;

    return VRTB_find_by_type_id_name(
        field_info->id, member_info.name,
        owner_info.body.custom.cs_id, var_info, &smt->v
    );
}

hir_subject_t* HIR_point_to_field(ast_node_t* root, hir_ctx_t* ctx, type_info_t* field_info, sym_table_t* smt) {
    hir_subject_t* base = NULL;
    if (
        !root->c || !root->c->t || 
        root->c->t->t_type != MEMBER_ACCESS_TOKEN
    ) {
        type_info_t ti;
        if (root->c) {
            if (root->c->t && root->c->t->t_type != INDEXATION_TOKEN)               base = HIR_generate_elem(root->c, ctx, smt);
            else if (TPTB_get_info_id(root->c->sinfo.t_id, &ti, &smt->t) && ti.ptr) base = HIR_generate_load_indexation(root->c, ctx, smt);
            else                                                                    base = HIR_generate_ref_indexation(root->c, ctx, smt);
        }
    }
    else {
        type_info_t parent_field;
        base = HIR_point_to_field(root->c, ctx, &parent_field, smt);
        variable_info_t parent_var;
        if (
            parent_field.t != TYPE_ARRAY &&
            HIR_find_member_variable(&parent_field, _member_owner_id(root->c), _member_name(root->c), &parent_var, smt) &&
            parent_var.vfs.ptr
        ) {
            token_t tmp = { .t_type = parent_var.type, .flags.ptr = parent_var.vfs.ptr };
            hir_subject_t* value = HIR_SUBJ_TMPVAR(HIR_get_tmptype_tkn(&tmp, 0), VRTB_add_info(NULL, tmp.t_type, NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v));
            value->ptr = tmp.flags.ptr;

            hir_subject_t* ref_base = HIR_SUBJ_TMPVAR(HIR_get_tmptype_tkn(&tmp, 0), VRTB_add_info(NULL, tmp.t_type, NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v));
            ref_base->ptr = base->ptr + 1;
            HIR_BLOCK2(ctx, HIR_TPTR, ref_base, base);

            HIR_BLOCK2(ctx, HIR_GDREF, value, ref_base);
            base = value;
        }
    }

    if (!base->ptr) {
        hir_subject_t* ref_base = HIR_reference_subject(base, smt, 1);
        HIR_BLOCK2(ctx, HIR_REF, ref_base, base);
        base = ref_base;
    }

    long offset = TPTB_get_child_offset_name(_member_owner_id(root), _member_name(root), &smt->t);
    if (offset == SMT_NULL) offset = TPTB_get_child_offset(root->c->sinfo.t_id, root->sinfo.t_id, &smt->t);
    TPTB_get_info_id(root->sinfo.t_id, field_info, &smt->t);
    return HIR_add_to_subject(base, smt, offset, ctx);
}

hir_subject_t* HIR_load_array_field_head(hir_subject_t* head, array_info_t* ai, hir_ctx_t* ctx, sym_table_t* smt) {
    token_t tmp = { .t_type = ai->elements_info.el_type };
    ai->elements_info.el_flags.ptr++;
    hir_subject_t* value = HIR_SUBJ_TMPVAR(
        HIR_get_tmptype_tkn(&tmp, 0), VRTB_add_info(NULL, tmp.t_type, NO_SYMBOL_ID, ai->elements_info.el_flags, &smt->v)
    );
    value->ptr = ai->elements_info.el_flags.ptr;
    HIR_BLOCK2(ctx, HIR_TPTR, value, head);
    return value;
}

hir_subject_t* HIR_generate_load_member_access(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    HIR_SET_CURRENT_POS(ctx, node);
    type_info_t ti;
    hir_subject_t* head = HIR_point_to_field(node, ctx, &ti, smt);

    array_info_t ai;
    variable_info_t vi;
    if (
        ti.t == TYPE_ARRAY                                                                  && 
        HIR_find_member_variable(&ti, _member_owner_id(node), _member_name(node), &vi, smt) &&
        ARTB_get_info(vi.v_id, &ai, &smt->a)
    ) return HIR_load_array_field_head(head, &ai, ctx, smt);
    
    if (!HIR_find_member_variable(&ti, _member_owner_id(node), _member_name(node), &vi, smt)) return NULL;
    token_t tmp = { .t_type = vi.type, .flags.ptr = vi.vfs.ptr };

    hir_subject_t* value = HIR_SUBJ_TMPVAR(HIR_get_tmptype_tkn(&tmp, 0), VRTB_add_info(NULL, tmp.t_type, NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v));
    value->ptr = tmp.flags.ptr;

    HIR_BLOCK2(ctx, HIR_GDREF, value, head);
    return value;
}

int HIR_generate_store_member_access(ast_node_t* node, hir_subject_t* data, hir_ctx_t* ctx, sym_table_t* smt) {
    HIR_SET_CURRENT_POS(ctx, node);
    type_info_t ti;
    hir_subject_t* head = HIR_point_to_field(node, ctx, &ti, smt);

    variable_info_t vi;
    if (!HIR_find_member_variable(&ti, _member_owner_id(node), _member_name(node), &vi, smt)) return 0;
    token_t tmp = { .t_type = vi.type, .flags.ptr = vi.vfs.ptr };

    /* If we're dealing with a pointer, we add one level of reference,
       given the nature of IR and because we need to preserve the pointer's
       level after container's dereference */
    if (tmp.flags.ptr) {
        head->ptr += tmp.flags.ptr;
    }

    hir_subject_t* target = HIR_SUBJ_TMPVAR(HIR_get_tmptype_tkn(&tmp, 0), VRTB_add_info(NULL, tmp.t_type, NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v));
    target->ptr = head->ptr;
    
    HIR_BLOCK2(ctx, HIR_TPTR, target, head);
    HIR_BLOCK2(ctx, HIR_LDREF, target, HIR_generate_implconv(ctx, tmp.flags.ptr, HIR_get_tmptype_tkn(&tmp, 0), data, smt));
    return 1;
}
