#ifndef TYPETB_H_
#define TYPETB_H_

#include <std/mm.h>
#include <std/map.h>
#include <std/set.h>
#include <std/str.h>
#include <std/list.h>
#include <prep/token_types.h>
#include <symtab/symtab_id.h>

typedef enum {
    TYPE_GENERICS,  /* <T>             */
    TYPE_CUSTOM,    /* container       */
    TYPE_PRIMITIVE, /* i32, i0..       */
    TYPE_METHOD,    /* function + self */
    TYPE_ARRAY,     /* array           */
    TYPE_SIGNATURE, /* signature       */
} type_type_t;

typedef struct {
    symbol_id_t parent;
    symbol_id_t child;
    string_t*   name;
} member_info_t;

typedef struct {
    list_t links; /* member_info_t* */
} member_t;

typedef struct {
    symbol_id_t                 id;
    symbol_id_t                 p;       /* parent of the copy  */
    string_t*                   name;    /* Type name           */
    symbol_id_t                 s_id;
    type_type_t                 t;       /* type's type         */
    int                         ptr;

    union {
        struct {
            list_t              arg_types; /* symbol_id_t */
            symbol_id_t         ret_type;
        } signature;
        struct {
            token_type_t        token;
        } generic;
        /* General container information */
        struct {
            symbol_id_t         cs_id;    // ChildScope Id
            struct {
                long            size;
                int             align;
                int             multiple; // Is this a union?
                signed char     vtable;   // Is this vtable container?
                list_t          children; // @items: symbol_id_t
            } layout;
        } custom;
        /* Method type stores the pointer to the
           linked function */
        struct {
            symbol_id_t         f_id; // Linked to a method function's id
            char                in_vtable : 1;
        } method;
        struct {
            token_type_t        token;
        } primitive;
        struct {
            symbol_id_t         element_t_id;
            long                size;
        } array;
    } body;
} type_info_t;

typedef struct {
    symbol_id_t curr_id;
    map_t       typetb;
    map_t       membtb;
} typetab_ctx_t;

int          TPTB_has_field(symbol_id_t c_id, symbol_id_t p_id, typetab_ctx_t* ctx);
int          TPTB_get_member_info(symbol_id_t p_id, symbol_id_t c_id, string_t* name, member_info_t* info, typetab_ctx_t* ctx);
int          TPTB_is_member(symbol_id_t c_id, typetab_ctx_t* ctx);
symbol_id_t  TPTB_get_signature(list_t* args, symbol_id_t ret, typetab_ctx_t* ctx);
symbol_id_t  TPTB_add_signature(list_t* args, symbol_id_t ret, typetab_ctx_t* ctx);
symbol_id_t  TPTB_resolve_parent(symbol_id_t c, typetab_ctx_t* ctx);
symbol_id_t  TPTB_add_info(string_t* name, symbol_id_t s_id, type_type_t t, int align, int multiple, int vtable, typetab_ctx_t* ctx);
symbol_id_t  TPTB_add_copy(symbol_id_t id, int ptr, typetab_ctx_t* ctx);
symbol_id_t  TPTB_add_info_from_token(symbol_id_t s_id, token_t* t, symbol_id_t f_id, typetab_ctx_t* ctx);
int          TPTB_set_as_vtable_method(symbol_id_t id, typetab_ctx_t* ctx);
int          TPTB_get_vtable_index(symbol_id_t p_id, symbol_id_t f_id, typetab_ctx_t* ctx);
long         TPTB_get_memory_size_id(symbol_id_t id, typetab_ctx_t* ctx);
int          TPTB_set_memory_size_id(symbol_id_t id, long size, typetab_ctx_t* ctx);
int          TPTB_set_child_scope_id(symbol_id_t id, symbol_id_t cs_id, typetab_ctx_t* ctx);
int          TPTB_link_child(symbol_id_t p_id, symbol_id_t c_id, typetab_ctx_t* ctx);
symbol_id_t  TPTB_get_first_child(symbol_id_t p_id, typetab_ctx_t* ctx);
symbol_id_t  TPTB_get_indexed_type(symbol_id_t id, typetab_ctx_t* ctx);
long         TPTB_get_child_offset(symbol_id_t p_id, symbol_id_t tc_id, typetab_ctx_t* ctx);
long         TPTB_get_child_offset_name(symbol_id_t p_id, string_t* name, typetab_ctx_t* ctx);
int          TPTB_add_as_child(symbol_id_t p_id, symbol_id_t c_id, string_t* name, long overrite_size, typetab_ctx_t* ctx);
int          TPTB_get_info_id(symbol_id_t id, type_info_t* info, typetab_ctx_t* ctx);
symbol_id_t  TPTB_resolve_child(symbol_id_t p_id, string_t* name, typetab_ctx_t* ctx);
type_type_t  TPTB_get_type_type_id(symbol_id_t id, typetab_ctx_t* ctx);
token_type_t TPTB_get_token_type_id(symbol_id_t id, typetab_ctx_t* ctx);
int          TPTB_get_info(string_t* name, symbol_id_t s_id, int ptr, type_info_t* info, typetab_ctx_t* ctx);

typedef struct {
    long        curr_idx;
    long        slot_off;
    long        slot_size;
    symbol_id_t slot_type;
    symbol_id_t slot_owner;
} type_init_info_t;

int TPTB_find_type_init_slot(symbol_id_t t_id, long target_slot, long base_offset, type_init_info_t* slot_info, typetab_ctx_t* ctx);
int TPTB_unload(typetab_ctx_t* ctx);

#endif
