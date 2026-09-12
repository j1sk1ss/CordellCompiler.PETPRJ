#ifndef ARRMEM_H_
#define ARRMEM_H_

#include <std/str.h>
#include <std/map.h>
#include <std/list.h>
#include <prep/token_types.h>
#include <symtab/symtab_id.h>

typedef enum {
    ARRAY_ELEM_STRING_TYPE,
    ARRAY_ELEM_CONST_TYPE,
    ARRAY_ELEM_FUNC_TYPE
} array_elem_type_t;

typedef struct {
    union {
        symbol_id_t   v_id;
        symbol_id_t   s_id;
        symbol_id_t   f_id;
        long long     value;
    } s;
    array_elem_type_t t;
} array_elem_info_t;

typedef struct {
    char                    vla : 1;
    symbol_id_t             v_id;
    long                    size;
    list_t                  elems; /* array_elem_info_t* */
    struct {
        token_type_t        el_type;
        basic_object_info_t el_flags;
    } elements_info;
} array_info_t;

typedef struct {
    map_t arrtb;
} arrtab_ctx_t;

int ARTB_get_info(symbol_id_t id, array_info_t* info, arrtab_ctx_t* ctx);
int ARTB_add_elems(symbol_id_t id, array_elem_info_t elem, arrtab_ctx_t* ctx);
symbol_id_t ARTB_add_info(symbol_id_t id, long size, int vla, token_type_t el_type, basic_object_info_t el_flags, arrtab_ctx_t* ctx);
symbol_id_t ARTB_add_copy(symbol_id_t nid, array_info_t* src, arrtab_ctx_t* ctx);
int ARTB_unload(arrtab_ctx_t* ctx);

#endif
