#include <hir/hirgens/hirgens.h>

/* Generate HIR for a string declaration.
Global strings are also copied into array metadata for static initialization.
Params:
    - `node` - Declaration AST node.
    - `ctx` - HIR context.
    - `smt` - Symtable.

Returns 1 if succeeds. */
static int _str_declaration(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    ast_node_t* name  = node->c;
    ast_node_t* size  = name->siblings.n;
    ast_node_t* type  = size->siblings.n;
    ast_node_t* value = type->siblings.n;
    HIR_BLOCK2(ctx, HIR_STRDECL, HIR_SUBJ_ASTVAR(name), HIR_SUBJ_STRING(value->c));

    long size_cap    = size->t->body->to_llong(size->t->body);
    int use_size_cap = size_cap != 0;

    variable_info_t vi;
    if (VRTB_get_info_id(name->sinfo.v_id, &vi, &smt->v) && vi.vfs.glob) {
        char* head = value->c->t->body->body;
        while (
            head && *head && 
            (!use_size_cap || size_cap-- > 0)
        ) ARTB_add_elems(vi.v_id, (array_elem_info_t){ .s.value = *(head++), .t = ARRAY_ELEM_CONST_TYPE }, &smt->a);
        ARTB_add_elems(vi.v_id, (array_elem_info_t){ .s.value = 0, .t = ARRAY_ELEM_CONST_TYPE }, &smt->a); /* C-string */
    }

    return 1;
}

/* Generate HIR subjects for declaration initializer elements.
Global initializers are copied into array metadata, while local initializers
are collected into a HIR subject list.
Params:
    - `vi` - Variable metadata for the declaration.
    - `elems` - AST node that contains initializer elements.
    - `ctx` - HIR context.
    - `smt` - Symtable.
    - `static_init` - Whether we're able to emit instructions for an element or not.

Returns a HIR subject list with local initializer elements. */
static hir_subject_t* _generate_init_args(variable_info_t* vi, ast_node_t* elems, hir_ctx_t* ctx, sym_table_t* smt, int static_init) {
    hir_subject_t* init_elems = HIR_SUBJ_LIST();
    if (!elems) return init_elems;
    for (ast_node_t* ast_el = elems->c; ast_el; ast_el = ast_el->siblings.n) {
        hir_subject_t* el = HIR_generate_elem(ast_el, ctx, smt);
        if (!el) continue;
        if (static_init) {
            if (HIR_is_defined_type(el->t)) ARTB_add_elems(vi->v_id, (array_elem_info_t){ .s.value = el->storage.num.value->to_llong(el->storage.num.value), .t = ARRAY_ELEM_CONST_TYPE  }, &smt->a);
            else if (el->t == HIR_STRING)   ARTB_add_elems(vi->v_id, (array_elem_info_t){ .s.s_id = el->storage.str.s_id,                                    .t = ARRAY_ELEM_STRING_TYPE }, &smt->a);
            else                            HIRGEN_ERROR(ctx, "Global initializer element must be a constant numeric value or string!");
            HIR_unload_subject(el);
        }
        else {
            hir_subject_t* curr_el = el;
            if (!HIR_is_defined_type(curr_el->t)) {
                HIR_BLOCK1(ctx, HIR_VRUSE, curr_el);
                curr_el = HIR_copy_subject(curr_el);
            }

            list_add(&init_elems->storage.list.h, curr_el);
        }
    }

    return init_elems;
} 

/* Generate HIR for an array declaration and its initializer.
Global arrays receive constant initializer values in array metadata; local
arrays keep runtime initializer subjects in the HIR declaration.
Params:
    - `node` - Declaration AST node.
    - `ctx` - HIR context.
    - `smt` - Symtable.

Returns 1 if succeeds. */
static int _arr_declaration(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    ast_node_t* name  = node->c;
    ast_node_t* size  = name->siblings.n;
    ast_node_t* type  = size->siblings.n;
    ast_node_t* elems = type->siblings.n;

    if (
        elems && elems->c                         && 
        elems->c->t->t_type == STRING_VALUE_TOKEN &&
        type->t->t_type == I8_TYPE_TOKEN          &&
        !type->t->flags.ptr
    ) return _str_declaration(node, ctx, smt);
    
    array_info_t ai;
    variable_info_t vi;
    if (
        VRTB_get_info_id(name->sinfo.v_id, &vi, &smt->v) && 
        ARTB_get_info(vi.v_id, &ai, &smt->a)
    ) {
        hir_subject_t* alloc_size = NULL;
        if (
            ai.vla || 
            vi.t_id == NO_SYMBOL_ID
        ) alloc_size = HIR_generate_elem(size, ctx, smt);
        else {
            long type_size = TPTB_get_memory_size_id(vi.t_id, &smt->t);
            if (type_size == SMT_NULL) type_size = 0;
            alloc_size = HIR_SUBJ_CONST(type_size);
        }

        HIR_BLOCK3(ctx, HIR_ARRDECL, HIR_SUBJ_ASTVAR(name), alloc_size, _generate_init_args(&vi, elems, ctx, smt, vi.vfs.glob));
    }

    return 1;
}

static void _load_vtable(hir_subject_t* args, type_info_t* ti, hir_ctx_t* ctx, sym_table_t* smt) {
    hir_subject_t* entry_elem = list_get_head(&args->storage.list.h);
    long slots = 0, fallback_index = 0;
    foreach (symbol_id_t c, &ti->body.custom.layout.children) {
        type_info_t c_ti;
        if (
            TPTB_get_info_id(c, &c_ti, &smt->t) && c_ti.t == TYPE_METHOD && 
            c_ti.body.method.in_vtable
        ) {
            long vtable_index = c_ti.body.method.vtable_index;
            if (vtable_index == SMT_NULL) vtable_index = fallback_index;
            if (vtable_index >= slots) slots = vtable_index + 1;
            fallback_index++;
        }
    }

    for (long slot = 0; slot < slots; slot++) {
        hir_subject_t* vtable_init = NULL;
        fallback_index = 0;
        foreach (symbol_id_t c, &ti->body.custom.layout.children) {
            type_info_t c_ti;
            func_info_t c_fi;
            if (
                !TPTB_get_info_id(c, &c_ti, &smt->t) ||
                c_ti.t != TYPE_METHOD               ||
                !c_ti.body.method.in_vtable
            ) continue;

            long vtable_index = c_ti.body.method.vtable_index;
            if (vtable_index == SMT_NULL) vtable_index = fallback_index;
            fallback_index++;
            if (vtable_index != slot) continue;
            if (!FNTB_get_info_id(c_ti.body.method.f_id, &c_fi, &smt->f) || c_fi.flags.abstract) break;

            vtable_init = HIR_SUBJ_TMPVAR(HIR_STKVARI0, VRTB_add_info(NULL, TMP_I0_TYPE_TOKEN, NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v));
            vtable_init->ptr = 1;
            HIR_BLOCK2(ctx, HIR_REF, vtable_init, HIR_SUBJ_FNAMETB(c_fi.id));
            break;
        }

        if (!vtable_init) vtable_init = HIR_SUBJ_CONST(0);
        list_insert(&args->storage.list.h, vtable_init, entry_elem);
    }
}

/* Generate allocation HIR for a custom container declaration.
Params:
    - `node` - Declaration AST node.
    - `ctx` - HIR context.
    - `smt` - Symtable.

Returns 1 if succeeds, otherwise 0. */
static int _cnt_declaration(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    ast_node_t* name  = node->c;
    ast_node_t* elems = name->siblings.n;
    type_info_t ti;
    if (!TPTB_get_info_id(TPTB_resolve_parent(node->sinfo.t_id, &smt->t), &ti, &smt->t)) return 0;
    variable_info_t vi;
    if (!VRTB_get_info_id(name->sinfo.v_id, &vi, &smt->v)) return 0;
    hir_subject_t* init_args = _generate_init_args(&vi, elems, ctx, smt, vi.vfs.glob || !TKN_in_stack(name->t));
    
    if (
        ti.t == TYPE_CUSTOM && init_args && 
        ti.body.custom.layout.vtable
    ) _load_vtable(init_args, &ti, ctx, smt);

    HIR_BLOCK3(ctx, HIR_ARRDECL, HIR_SUBJ_ASTVAR(node->c), HIR_SUBJ_CONST(TPTB_get_memory_size_id(ti.id, &smt->t)), init_args);
    return 1;
}

/* Dispatch declarations that need storage larger than a scalar stack slot.
Params:
    - `node` - Declaration AST node.
    - `ctx` - HIR context.
    - `smt` - Symtable.

Returns 1 if succeeds. */
static inline int _starr_declaration(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    switch (node->t->t_type) {
        case ARRAY_TYPE_TOKEN:  return _arr_declaration(node, ctx, smt);
        case CUSTOM_TYPE_TOKEN: return _cnt_declaration(node, ctx, smt);
        default:                return 0;
    }
}

int HIR_generate_declaration_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt) {
    HIR_SET_CURRENT_POS(ctx, node);
    ast_node_t* name = node->c;
    if (!TKN_is_one_slot(name->t)) {
        return _starr_declaration(node, ctx, smt);
    }

    /* Do not declare a variable if it is global.
       All essential info already in the symtable. */
    if (!TKN_in_stack(name->t)) {
        variable_info_t vi;
        if (
            VRTB_get_info_id(name->sinfo.v_id, &vi, &smt->v) && 
            vi.vfs.glob && name->siblings.n
        ) VRTB_update_definition(vi.v_id, name->siblings.n->c->t->body->to_llong(name->siblings.n->c->t->body), NO_SYMBOL_ID, &smt->v, 0);
        return 1;
    }
    
    HIR_BLOCK1(ctx, HIR_VARDECL, HIR_SUBJ_ASTVAR(name));
    HAS_ANNOTATION(POPARG_ANNOTATION, node, {
        if (!ctx->carry.varg) {
            HIRGEN_ERROR(ctx, POPRG_ANNOTATION_COMMAND " can't be used in this function!");
            return 0;
        }

        hir_subject_t* decl = HIR_SUBJ_ASTVAR(name);
        HIR_BLOCK2(ctx, HIR_GDREF, decl, HIR_copy_subject(ctx->carry.varg));
        hir_subject_t* res = HIR_SUBJ_TMPVAR(
            ctx->carry.varg->t, 
            VRTB_add_info(NULL, HIR_get_tmptkn_type(ctx->carry.varg->t), NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v)
        );
        res->ptr = ctx->carry.varg->ptr;
        HIR_BLOCK3(
            ctx, HIR_iADD, res, HIR_copy_subject(ctx->carry.varg), 
            HIR_SUBJ_CONST(CONF_get_full_bytness())
        );
        HIR_BLOCK2(ctx, HIR_STORE, HIR_copy_subject(ctx->carry.varg), res);
        return 1;
    });

    HAS_ANNOTATION(POPREG_ANNOTATION, node, {
        hir_subject_t* decl = HIR_SUBJ_ASTVAR(name);
        hir_subject_t* reg_src = HIR_SUBJ_TMPVAR(
            HIR_get_tmp_type(decl->t), 
            VRTB_add_info(NULL, TKN_get_tmp_type(name->t->t_type), NO_SYMBOL_ID, EMPTY_BASIC_FLAGS, &smt->v)
        );
        variable_info_t vi;
        if (
            !VRTB_get_info_id(reg_src->storage.var.v_id, &vi, &smt->v) ||
            !VRTB_update_memory(vi.v_id, FIELD_NO_CHANGE, FIELD_NO_CHANGE, annot->data.regval, FIELD_NO_CHANGE, &smt->v)
        ) return 0;
        HIR_BLOCK2(ctx, HIR_STORE, decl, reg_src);
        return 1;
    });

    if (!name->siblings.n) return 1;
    hir_subject_t* src = HIR_generate_elem(name->siblings.n->c, ctx, smt);
    if (!src) {
        HIRGEN_ERROR(ctx, "Assign: The right part generation error!");
        return 0;
    }
    
    return HIR_generate_store_block(name, src, ctx, smt);
}
