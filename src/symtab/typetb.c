#include <symtab/typetb.h>

static inline token_type_t _default_token_type(type_type_t t) {
    switch (t) {
        case TYPE_GENERICS:  return GENERIC_TYPE_TOKEN;
        case TYPE_CUSTOM:    return CUSTOM_TYPE_TOKEN;
        case TYPE_METHOD:    return FUNC_TOKEN;
        case TYPE_ARRAY:     return ARRAY_TYPE_TOKEN;
        case TYPE_SIGNATURE: return SIGNATURE_TOKEN;
        case TYPE_PRIMITIVE:
        default:             return UNKNOWN_STRING_TOKEN;
    }
}

static type_info_t* _create_type_info(string_t* name) {
    type_info_t* info = (type_info_t*)mm_malloc(sizeof(type_info_t));
    if (!info) return NULL;

    str_memset(info, 0, sizeof(type_info_t));
    info->id            = NO_SYMBOL_ID;
    info->p             = NO_SYMBOL_ID;
    info->s_id          = NO_SYMBOL_ID;
    info->ptr           = 0;

    if (name) info->name = name->copy(name);
    else      info->name = NULL;
    return info;
}

static member_t* _create_member() {
    member_t* info = (member_t*)mm_malloc(sizeof(member_t));
    if (!info) return NULL;
    list_init(&info->links);
    return info;
}

static member_info_t* _create_member_info(symbol_id_t p_id, symbol_id_t c_id, string_t* name) {
    member_info_t* info = (member_info_t*)mm_malloc(sizeof(member_info_t));
    if (!info) return NULL;

    info->parent = p_id;
    info->child  = c_id;
    info->name   = name ? name->copy(name) : NULL;
    return info;
}

static inline member_t* _get_member(symbol_id_t p_id, typetab_ctx_t* ctx) {
    member_t* member;
    if (map_get(&ctx->membtb, p_id, (void**)&member)) return member;
    return NULL;
}

static member_t* _get_or_create_member(symbol_id_t p_id, typetab_ctx_t* ctx) {
    member_t* member = _get_member(p_id, ctx);
    if (member) return member;

    member = _create_member();
    if (!member || !map_put(&ctx->membtb, p_id, member)) {
        if (member) mm_free(member);
        return NULL;
    }

    return member;
}

static inline int _member_name_equals(member_info_t* info, string_t* name) {
    if (!name) return 1;
    return info->name && info->name->equals(info->name, name);
}

static member_info_t* _find_member_info(symbol_id_t p_id, symbol_id_t c_id, string_t* name, typetab_ctx_t* ctx) {
    if (p_id != NO_SYMBOL_ID) {
        p_id = TPTB_resolve_parent(p_id, ctx);
        member_t* member = _get_member(p_id, ctx);
        if (!member) return NULL;

        foreach (member_info_t* info, &member->links) {
            if (c_id != NO_SYMBOL_ID && info->child != c_id) continue;
            if (!_member_name_equals(info, name)) continue;
            return info;
        }

        return NULL;
    }

    map_foreach (member_t* member, &ctx->membtb) {
        foreach (member_info_t* info, &member->links) {
            if (c_id != NO_SYMBOL_ID && info->child != c_id) continue;
            if (!_member_name_equals(info, name)) continue;
            return info;
        }
    }

    return NULL;
}

static int _add_member_info(symbol_id_t p_id, symbol_id_t c_id, string_t* name, typetab_ctx_t* ctx) {
    p_id = TPTB_resolve_parent(p_id, ctx);

    member_t* member = _get_or_create_member(p_id, ctx);
    if (!member) return 0;

    member_info_t* info = _create_member_info(p_id, c_id, name);
    if (!info) return 0;
    if (list_add(&member->links, info)) return 1;

    if (info->name) destroy_string(info->name);
    mm_free(info);
    return 0;
}

static void _init_type_body(type_info_t* info, type_type_t t, token_type_t token) {
    info->t = t;
    switch (t) {
        case TYPE_GENERICS: info->body.generic.token = token; break;
        case TYPE_CUSTOM: {
            info->body.custom.cs_id           = NO_SYMBOL_ID;
            info->body.custom.layout.size     = 0;
            info->body.custom.layout.align    = CONF_get_eight_bytness();
            info->body.custom.layout.multiple = 0;
            list_init(&info->body.custom.layout.children);
            break;
        }
        case TYPE_METHOD: info->body.method.f_id = NO_SYMBOL_ID; break;
        case TYPE_ARRAY: {
            info->body.array.element_t_id = NO_SYMBOL_ID;
            info->body.array.size         = 0;
            break;
        }
        case TYPE_SIGNATURE: {
            list_init(&info->body.signature.arg_types);
            info->body.signature.ret_type = NO_SYMBOL_ID;
            break;
        }
        case TYPE_PRIMITIVE:
        default: {
            info->body.primitive.token = token;
            break;
        }
    }
}

static inline list_t* _type_children(type_info_t* info) {
    if (!info) return NULL;
    switch (info->t) {
        case TYPE_CUSTOM: return &info->body.custom.layout.children;
        default:          return NULL;
    }
}

static inline token_type_t _type_token_type(const type_info_t* info) {
    if (!info) return UNKNOWN_STRING_TOKEN;
    switch (info->t) {
        case TYPE_GENERICS:  return info->body.generic.token;
        case TYPE_CUSTOM:    return CUSTOM_TYPE_TOKEN;
        case TYPE_METHOD:    return FUNC_TOKEN;
        case TYPE_ARRAY:     return ARRAY_TYPE_TOKEN;
        case TYPE_SIGNATURE: return SIGNATURE_TOKEN;
        case TYPE_PRIMITIVE: return info->body.primitive.token;
        default:             return UNKNOWN_STRING_TOKEN;
    }
}

static inline void _set_type_size(type_info_t* info, long size) {
    if (!info) return;
    switch (info->t) {
        case TYPE_CUSTOM: info->body.custom.layout.size = size; break;
        case TYPE_ARRAY:  info->body.array.size         = size; break;
        default:                                                break;
    }
}

static inline long _raw_type_size(type_info_t* info, int vtable) {
    if (!info) return SMT_NULL;
    switch (info->t) {
        case TYPE_PRIMITIVE: {
            token_t token = { .t_type = info->body.primitive.token };
            return TKN_convert_type_size(TKN_variable_bitness(&token, 0));
        }
        case TYPE_CUSTOM: return info->body.custom.layout.size;
        case TYPE_ARRAY:  return info->body.array.size;
        case TYPE_METHOD: if (vtable) return CONF_get_full_bytness();
        case TYPE_GENERICS:
        case TYPE_SIGNATURE:
        default:          return SMT_NULL;
    }
}

static inline long _type_memory_size(type_info_t* info, int vtable) {
    if (!info) return SMT_NULL;
    return info->ptr ? CONF_get_full_bytness() : _raw_type_size(info, vtable);
}

static void _copy_type_body(type_info_t* dst, const type_info_t* src) {
    _init_type_body(dst, src->t, _type_token_type(src));
    switch (src->t) {
        case TYPE_CUSTOM: {
            dst->body.custom.cs_id           = src->body.custom.cs_id;
            dst->body.custom.layout.size     = src->body.custom.layout.size;
            dst->body.custom.layout.align    = src->body.custom.layout.align;
            dst->body.custom.layout.multiple = src->body.custom.layout.multiple;
            break;
        }
        case TYPE_METHOD: dst->body.method.f_id = src->body.method.f_id; break;
        case TYPE_ARRAY: {
            dst->body.array.element_t_id = src->body.array.element_t_id;
            dst->body.array.size         = src->body.array.size;
            break;
        }
        case TYPE_SIGNATURE: {
            dst->body.signature.ret_type = src->body.signature.ret_type;
            list_copy((list_t*)&src->body.signature.arg_types, &dst->body.signature.arg_types);
            break;
        }
        case TYPE_GENERICS:
        case TYPE_PRIMITIVE:
        default:                                                         break;
    }
}

symbol_id_t TPTB_get_signature(list_t* args, symbol_id_t ret, typetab_ctx_t* ctx) {
    map_foreach (type_info_t* ti, &ctx->typetb) {
        if (
            ti->t != TYPE_SIGNATURE            ||
            ti->body.signature.ret_type != ret ||
            list_size(args) != list_size(&ti->body.signature.arg_types)
        ) continue;
        
        int same = 1;
        list_iter_t expected;
        list_iter_t provided;
        list_iter_hinit(&ti->body.signature.arg_types, &expected);
        list_iter_hinit(args, &provided);

        void* expected_id;
        void* provided_id;
        while (
            list_iter_next(&expected, &expected_id) &&
            list_iter_next(&provided, &provided_id)
        ) {
            if ((symbol_id_t)expected_id != (symbol_id_t)provided_id) {
                same = 0;
                break;
            }
        }

        if (!same) continue;
        return ti->id;
    }

    return NO_SYMBOL_ID;
}

symbol_id_t TPTB_add_signature(list_t* args, symbol_id_t ret, typetab_ctx_t* ctx) {
    if (TPTB_get_signature(args, ret, ctx) != NO_SYMBOL_ID) return NO_SYMBOL_ID;

    type_info_t* info = _create_type_info(NULL);
    if (!info) return NO_SYMBOL_ID;

    info->id = ctx->curr_id++;
    _init_type_body(info, TYPE_SIGNATURE, SIGNATURE_TOKEN);
    foreach(symbol_id_t arg, args) {
        list_add(&info->body.signature.arg_types, (void*)arg);
    }

    info->body.signature.ret_type = ret;
    map_put(&ctx->typetb, info->id, info);
    return info->id;
}

symbol_id_t TPTB_add_info(string_t* name, symbol_id_t s_id, type_type_t t, int align, int multiple, int vtable, typetab_ctx_t* ctx) {
    if (TPTB_get_info(name, s_id, 0, NULL, ctx)) return NO_SYMBOL_ID;
    type_info_t* info = _create_type_info(name);
    if (!info) return NO_SYMBOL_ID;

    info->id   = ctx->curr_id++;
    info->s_id = s_id;
    _init_type_body(info, t, _default_token_type(t));

    if (t == TYPE_CUSTOM) {
        info->body.custom.layout.align    = align;
        info->body.custom.layout.multiple = multiple;
        info->body.custom.layout.vtable   = vtable;
    }

    map_put(&ctx->typetb, info->id, info);
    return info->id;
}

static symbol_id_t _get_existing_copy(symbol_id_t id, int ptr, typetab_ctx_t* ctx) {
    symbol_id_t root_id = TPTB_resolve_parent(id, ctx);
    map_foreach (type_info_t* ti, &ctx->typetb) {
        if (
            ti->ptr == ptr                              && // same ptr
            TPTB_resolve_parent(ti->id, ctx) == root_id && // same parent
            ti->p != NO_SYMBOL_ID                          // its a copy
        ) return ti->id;
    }

    return NO_SYMBOL_ID;
}

symbol_id_t TPTB_add_copy(symbol_id_t id, int ptr, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (!map_get(&ctx->typetb, id, (void**)&ti)) return NO_SYMBOL_ID;
    if (ti->t == TYPE_GENERICS) return id;

    symbol_id_t existed_copy = _get_existing_copy(id, ptr, ctx);
    if (existed_copy != NO_SYMBOL_ID) return existed_copy;

    type_info_t* info = _create_type_info(ti->name);
    if (!info) return NO_SYMBOL_ID;

    info->p        = id;
    info->s_id     = ti->s_id;
    info->ptr      = ptr;
    _copy_type_body(info, ti);

    info->id = ctx->curr_id++;
    map_put(&ctx->typetb, info->id, info);
    return info->id;
}

static inline symbol_id_t _get_type_by_token(token_t* t, typetab_ctx_t* ctx) {
    if (!t) return NO_SYMBOL_ID;
    if (t->t_type == ARRAY_TYPE_TOKEN) return NO_SYMBOL_ID;
    map_foreach (type_info_t* ti, &ctx->typetb) {
        if (_type_token_type(ti) != t->t_type || ti->ptr != t->flags.ptr) continue;
        if (ti->p != NO_SYMBOL_ID)                                        continue;
        if (ti->t == TYPE_METHOD && ti->body.method.f_id != NO_SYMBOL_ID) continue;
        if (!t->body && !ti->name)                                        return ti->id;
        if (t->body && ti->name && t->body->equals(t->body, ti->name))    return ti->id;
    }

    return NO_SYMBOL_ID;
}

static inline symbol_id_t _get_type_by_fid(symbol_id_t f_id, typetab_ctx_t* ctx) {
    map_foreach (type_info_t* ti, &ctx->typetb) {
        if (ti->t != TYPE_METHOD || ti->body.method.f_id == NO_SYMBOL_ID) continue;
        if (ti->body.method.f_id == f_id && f_id != NO_SYMBOL_ID) return ti->id;
    }

    return NO_SYMBOL_ID;
}

symbol_id_t TPTB_add_info_from_token(symbol_id_t s_id, token_t* t, symbol_id_t f_id, typetab_ctx_t* ctx) {
    type_type_t type_kind;
    switch (t->t_type) {
        case GENERIC_TYPE_TOKEN:
        case GENERIC_VARIABLE_TOKEN: type_kind = TYPE_GENERICS;  break;
        case CUSTOM_TYPE_TOKEN:
        case CUSTOM_VARIABLE_TOKEN:  type_kind = TYPE_CUSTOM;    break;
        case FUNC_TOKEN:
        case FUNC_PROT_TOKEN:        type_kind = TYPE_METHOD;    break;
        case ARRAY_TYPE_TOKEN:       type_kind = TYPE_ARRAY;     break;
        default:                     type_kind = TYPE_PRIMITIVE; break;
    }

    int linked_method = type_kind == TYPE_METHOD && f_id != NO_SYMBOL_ID;
    symbol_id_t existed = linked_method ? _get_type_by_fid(f_id, ctx) : _get_type_by_token(t, ctx);
    if (existed != NO_SYMBOL_ID) return existed;

    type_info_t* info = _create_type_info(t->body);
    if (!info) return NO_SYMBOL_ID;

    info->id   = ctx->curr_id++;
    info->s_id = s_id;
    info->ptr  = t->flags.ptr;

    _init_type_body(info, type_kind, t->t_type);
    if (type_kind == TYPE_METHOD) info->body.method.f_id = f_id;

    map_put(&ctx->typetb, info->id, info);
    return info->id;
}

int TPTB_has_field(symbol_id_t c_id, symbol_id_t p_id, typetab_ctx_t* ctx) {
    return _find_member_info(p_id, c_id, NULL, ctx) != NULL;
}

int TPTB_get_member_info(symbol_id_t p_id, symbol_id_t c_id, string_t* name, member_info_t* info, typetab_ctx_t* ctx) {
    member_info_t* found = _find_member_info(p_id, c_id, name, ctx);
    if (!found) return 0;
    if (info) str_memcpy(info, found, sizeof(member_info_t));
    return 1;
}

int TPTB_is_member(symbol_id_t c_id, typetab_ctx_t* ctx) {
    return _find_member_info(NO_SYMBOL_ID, c_id, NULL, ctx) != NULL;
}

long TPTB_get_memory_size_id(symbol_id_t id, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (!map_get(&ctx->typetb, id, (void**)&ti)) return SMT_NULL;
    return _type_memory_size(ti, 0);
}

int TPTB_set_memory_size_id(symbol_id_t id, long size, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (!map_get(&ctx->typetb, id, (void**)&ti)) return 0;
    _set_type_size(ti, size);
    return 1;
}

int TPTB_set_child_scope_id(symbol_id_t id, symbol_id_t cs_id, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (!map_get(&ctx->typetb, id, (void**)&ti) || ti->t != TYPE_CUSTOM) return 0;
    ti->body.custom.cs_id = cs_id;
    return 1;
}

symbol_id_t TPTB_resolve_parent(symbol_id_t c, typetab_ctx_t* ctx) {
    type_info_t* c_ti;
    while (
        map_get(&ctx->typetb, c, (void**)&c_ti) &&
        c_ti->p != NO_SYMBOL_ID
    ) c = c_ti->p;
    return c;
}

int TPTB_link_child(symbol_id_t p_id, symbol_id_t c_id, typetab_ctx_t* ctx) {
    p_id = TPTB_resolve_parent(p_id, ctx);

    type_info_t *p_ti, *c_ti;
    if (
        p_id != c_id &&
        map_get(&ctx->typetb, p_id, (void**)&p_ti) &&
        map_get(&ctx->typetb, c_id, (void**)&c_ti)
    ) {
        if (p_ti->t == TYPE_ARRAY) {
            p_ti->body.array.element_t_id = c_id;
            return 1;
        }

        if (p_ti->t != TYPE_CUSTOM) return 0;
        list_add(&p_ti->body.custom.layout.children, (void*)c_id);
        if (p_ti->body.custom.cs_id == NO_SYMBOL_ID) p_ti->body.custom.cs_id = c_ti->s_id;
        return 1;
    }

    return 0;
}

symbol_id_t TPTB_get_first_child(symbol_id_t p_id, typetab_ctx_t* ctx) {
    p_id = TPTB_resolve_parent(p_id, ctx);
    type_info_t* p_ti;
    if (!map_get(&ctx->typetb, p_id, (void**)&p_ti)) return NO_SYMBOL_ID;

    if (p_ti->t == TYPE_ARRAY) return p_ti->body.array.element_t_id;
    if (p_ti->t != TYPE_CUSTOM || !p_ti->body.custom.layout.children.s) return NO_SYMBOL_ID;
    return (symbol_id_t)list_get_head(&p_ti->body.custom.layout.children);
}

symbol_id_t TPTB_get_indexed_type(symbol_id_t id, typetab_ctx_t* ctx) {
    type_info_t ti;
    if (!TPTB_get_info_id(id, &ti, ctx)) return NO_SYMBOL_ID;
    if (ti.ptr) return TPTB_add_copy(id, ti.ptr - 1, ctx);
    if (ti.t == TYPE_ARRAY) return TPTB_get_first_child(id, ctx);
    return NO_SYMBOL_ID;
}

int TPTB_add_as_child(symbol_id_t p_id, symbol_id_t c_id, string_t* name, long overrite_size, typetab_ctx_t* ctx) {
    if (p_id == NO_SYMBOL_ID) return 0;
    p_id = TPTB_resolve_parent(p_id, ctx);

    type_info_t *p_ti, *c_ti;
    if (
        p_id != c_id &&
        map_get(&ctx->typetb, p_id, (void**)&p_ti) &&
        map_get(&ctx->typetb, c_id, (void**)&c_ti)
    ) {
        if (p_ti->t != TYPE_CUSTOM) return 0;
        if (name && _find_member_info(p_id, NO_SYMBOL_ID, name, ctx)) return 1;
        if (!_add_member_info(p_id, c_id, name, ctx)) return 0;

        list_add(&p_ti->body.custom.layout.children, (void*)c_id);
        
        long field_size = _type_memory_size(c_ti, p_ti->body.custom.layout.vtable);
        if (overrite_size != FIELD_NO_CHANGE) {
            field_size = overrite_size;
            if (!c_ti->ptr) _set_type_size(c_ti, overrite_size);
        }

        if (!p_ti->body.custom.layout.multiple)        p_ti->body.custom.layout.size = MAX(p_ti->body.custom.layout.size, ALIGN(field_size, p_ti->body.custom.layout.align));
        else if (p_ti->body.custom.layout.align != -1) p_ti->body.custom.layout.size += ALIGN(field_size, p_ti->body.custom.layout.align);
        else                                           p_ti->body.custom.layout.size += ALIGN(field_size, field_size);
        return 1;
    }

    return 0;
}

symbol_id_t TPTB_resolve_child(symbol_id_t p_id, string_t* name, typetab_ctx_t* ctx) {
    member_info_t* info = _find_member_info(p_id, NO_SYMBOL_ID, name, ctx);
    return info ? info->child : NO_SYMBOL_ID;
}

long TPTB_get_child_offset(symbol_id_t p_id, symbol_id_t tc_id, typetab_ctx_t* ctx) {
    p_id = TPTB_resolve_parent(p_id, ctx);

    type_info_t *p_ti, *c_ti;
    if (!map_get(&ctx->typetb, p_id, (void**)&p_ti)) return SMT_NULL;
    if (p_ti->t != TYPE_CUSTOM || !p_ti->body.custom.layout.multiple) return 0;

    long offset = 0;
    foreach (symbol_id_t c_id, &p_ti->body.custom.layout.children) {
        if (!map_get(&ctx->typetb, c_id, (void**)&c_ti)) continue;
        long size = _type_memory_size(c_ti, p_ti->body.custom.layout.vtable);
        if (p_ti->body.custom.layout.align == -1) {
            offset = ALIGN(offset, size);
            if (tc_id == c_id) return offset;
            offset += size;
        }
        else {
            if (tc_id == c_id) return offset;
            offset += ALIGN(size, p_ti->body.custom.layout.align);
        }
    }

    return SMT_NULL;
}

long TPTB_get_child_offset_name(symbol_id_t p_id, string_t* name, typetab_ctx_t* ctx) {
    p_id = TPTB_resolve_parent(p_id, ctx);

    type_info_t *p_ti, *c_ti;
    if (!map_get(&ctx->typetb, p_id, (void**)&p_ti)) return SMT_NULL;
    if (p_ti->t != TYPE_CUSTOM || !p_ti->body.custom.layout.multiple) return 0;

    member_t* member = _get_member(p_id, ctx);
    if (!member) return SMT_NULL;

    long offset = 0;
    foreach (member_info_t* info, &member->links) {
        if (!map_get(&ctx->typetb, info->child, (void**)&c_ti)) continue;
        long size = _type_memory_size(c_ti, p_ti->body.custom.layout.vtable);
        if (p_ti->body.custom.layout.align == -1) {
            offset = ALIGN(offset, size);
            if (_member_name_equals(info, name)) return offset;
            offset += size;
        }
        else {
            if (_member_name_equals(info, name)) return offset;
            offset += ALIGN(size, p_ti->body.custom.layout.align);
        }
    }

    return SMT_NULL;
}

int TPTB_find_type_init_slot(symbol_id_t t_id, long target_slot, long base_offset, type_init_info_t* slot_info, typetab_ctx_t* ctx) {
    type_info_t ti;
    if (!TPTB_get_info_id(t_id, &ti, ctx)) return 0;

    long type_size = TPTB_get_memory_size_id(t_id, ctx);
    if (ti.ptr || ti.t == TYPE_PRIMITIVE) {
        if (slot_info->curr_idx++ != target_slot) return 0;
        slot_info->slot_off   = base_offset;
        slot_info->slot_size  = type_size;
        slot_info->slot_type  = t_id;
        slot_info->slot_owner = t_id;
        return type_size > 0;
    }

    symbol_id_t scan_id = ti.p != NO_SYMBOL_ID ? TPTB_resolve_parent(t_id, ctx) : t_id;
    type_info_t scan_ti;
    if (!TPTB_get_info_id(scan_id, &scan_ti, ctx)) return 0;

    if (scan_ti.t == TYPE_ARRAY) {
        symbol_id_t child_id = scan_ti.body.array.element_t_id;
        long child_size = TPTB_get_memory_size_id(child_id, ctx);
        if (child_size <= 0) return 0;
        long repeats = type_size / child_size;
        for (long repeat = 0; repeat < repeats; repeat++) {
            long nested_base = base_offset + repeat * child_size;
            if (TPTB_find_type_init_slot(child_id, target_slot, nested_base, slot_info, ctx)) {
                slot_info->slot_owner = scan_id;
                return 1;
            }
        }

        return 0;
    }

    if (scan_ti.t != TYPE_CUSTOM) return 0;

    long child_offset = 0;
    foreach (symbol_id_t child_id, &scan_ti.body.custom.layout.children) {
        long child_size = TPTB_get_memory_size_id(child_id, ctx);
        if (child_offset < 0 || child_size <= 0) continue;
        if (scan_ti.body.custom.layout.align == -1) child_offset = ALIGN(child_offset, child_size);
        if (TPTB_find_type_init_slot(child_id, target_slot, base_offset + child_offset, slot_info, ctx)) return 1;
        if (scan_ti.body.custom.layout.align == -1) child_offset += child_size;
        else child_offset += ALIGN(child_size, scan_ti.body.custom.layout.align);
    }

    return 0;
}

int TPTB_get_info_id(symbol_id_t id, type_info_t* info, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (map_get(&ctx->typetb, id, (void**)&ti)) {
        if (info) str_memcpy(info, ti, sizeof(type_info_t));
        return 1;
    }

    return 0;
}

int TPTB_get_vtable_index(symbol_id_t p_id, symbol_id_t f_id, typetab_ctx_t* ctx) {
    type_info_t *p_ti, *c_ti;
    if (map_get(&ctx->typetb, p_id, (void**)&p_ti) && p_ti->t == TYPE_CUSTOM) {
        int i = 0;
        foreach (symbol_id_t rc_id, &p_ti->body.custom.layout.children) {
            if (
                map_get(&ctx->typetb, rc_id, (void**)&c_ti) &&
                c_ti->t == TYPE_METHOD                      &&
                c_ti->body.method.f_id == f_id
            ) return i;
            i++;
        }
    }

    return SMT_NULL;
}

type_type_t TPTB_get_type_type_id(symbol_id_t id, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (map_get(&ctx->typetb, id, (void**)&ti)) return ti->t;
    return 0;
}

token_type_t TPTB_get_token_type_id(symbol_id_t id, typetab_ctx_t* ctx) {
    type_info_t* ti;
    if (map_get(&ctx->typetb, id, (void**)&ti)) return _type_token_type(ti);
    return CUSTOM_TYPE_TOKEN;
}

int TPTB_get_info(string_t* name, symbol_id_t s_id, int ptr, type_info_t* info, typetab_ctx_t* ctx) {
    map_foreach (type_info_t* ti, &ctx->typetb) {
        if (
            (ti->t == TYPE_METHOD && ti->body.method.f_id != NO_SYMBOL_ID) || // this is a method
            ti->ptr != ptr
        ) continue;
        if (
            name && ti->name && name->equals(name, ti->name) &&
            (s_id == ti->s_id || ti->s_id == NO_SYMBOL_ID)
        ) {
            if (info) str_memcpy(info, ti, sizeof(type_info_t));
            return 1;
        }
    }

    return 0;
}

static int _unload_info(type_info_t* info) {
    list_t* children = _type_children(info);
    if (children)                  list_free(children);
    if (info->t == TYPE_SIGNATURE) list_free(&info->body.signature.arg_types);
    if (info->name)                destroy_string(info->name);
    return mm_free(info);
}

static int _unload_member_info(member_info_t* info) {
    if (info->name) destroy_string(info->name);
    return mm_free(info);
}

static int _unload_member(member_t* info) {
    list_free_force_op(&info->links, (int (*)(void*))_unload_member_info);
    return mm_free(info);
}

int TPTB_unload(typetab_ctx_t* ctx) {
    return
        map_free_force_op(&ctx->membtb, (int (*)(void*))_unload_member) &&
        map_free_force_op(&ctx->typetb, (int (*)(void*))_unload_info);
}
