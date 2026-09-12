#include <hir/hirgens/hirgens.h>

hir_subject_t* HIR_add_to_subject(hir_subject_t* src, sym_table_t* smt, long add, hir_ctx_t* ctx) {
    hir_subject_t* add_subj = HIR_SUBJ_TMPVAR(
        src->t, VRTB_add_info(NULL, HIR_get_tmptkn_type(src->t), NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v)
    );
    add_subj->ptr = src->ptr;
    HIR_BLOCK3(ctx, HIR_iADD, add_subj, src, HIR_SUBJ_CONST(add));
    return add_subj;
}

hir_subject_t* HIR_gdref_subject(hir_subject_t* src, sym_table_t* smt, hir_ctx_t* ctx) {
    hir_subject_t* dref_subj = HIR_SUBJ_TMPVAR(
        src->t, VRTB_add_info(NULL, HIR_get_tmptkn_type(src->t), NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v)
    );
    dref_subj->ptr = src->ptr - 1;
    HIR_BLOCK2(ctx, HIR_GDREF, dref_subj, src);
    return dref_subj;
}
