#ifndef LIR_H_
#define LIR_H_

#include <utils.h>
#include <position.h>
#include <std/mm.h>
#include <std/str.h>
#include <std/mem.h>
#include <std/map.h>
#include <std/list.h>
#include <std/stack.h>
#include <std/stackmap.h>
#include <symtab/symtab_id.h>
#include <lir/lir_types.h>

typedef struct {
    long value;
} lir_constant_t;

typedef struct {
    lir_registers_t base;
    int             offset;
    symbol_id_t     v_id;
} lir_variable_t;

typedef struct {
    lir_registers_t reg;
} lir_register_t;

typedef struct {
    char      is_float : 1;
    string_t* value;
} lir_number_t;

typedef struct {
    long lb_id;
} lir_label_t;

typedef struct {
    char        rel : 1;
    symbol_id_t sid;
} lir_str_t;

typedef struct {
    list_t h;
} lir_list_t;

typedef struct {
    unsigned long       id;
    struct lir_block*   home;
    char                size;  /* Size of a subject              */
    char                dsize; /* Dereferenced size of a subject */
    lir_subject_type_t  t;
    union {
        lir_constant_t  cnst;
        lir_variable_t  var;
        lir_register_t  reg;
        lir_number_t    num;
        lir_label_t     lb;
        lir_str_t       str;
        lir_list_t      list;
        file_position_t pos;
    } storage;
} lir_subject_t;

typedef struct lir_block {
    char              unused;
    struct lir_block* prev;
    struct lir_block* next;
    lir_operation_t   op;
    lir_subject_t*    farg;
    lir_subject_t*    sarg;
    lir_subject_t*    targ;
    int               args;
} lir_block_t;

typedef struct {
    long          cid;
    int           lid;
    lir_block_t*  h;
    lir_block_t*  t;
    stack_map_t   stk;
    map_t*        vars;
} lir_ctx_t;

lir_block_t* LIR_create_block(lir_operation_t op, lir_subject_t* fa, lir_subject_t* sa, lir_subject_t* ta);
lir_subject_t* LIR_create_subject(lir_subject_type_t t, int reg, int v_id, long offset, string_t* strval, long intval, int size);
lir_subject_t* LIR_copy_subject(lir_subject_t* s);
int LIR_unlink_block(lir_block_t* block);
int LIR_insert_block_after(lir_block_t* block, lir_block_t* pos);
int LIR_insert_block_before(lir_block_t* block, lir_block_t* pos);
int LIR_append_block(lir_block_t* block, lir_ctx_t* ctx);
int LIR_subj_equals(lir_subject_t* a, lir_subject_t* b);
int LIR_unload_subject(lir_subject_t* s);
int LIR_unload_blocks(lir_block_t* block);
                                                      /* LIR type     Reg    vID              Offset  String            Int  Size */
#define LIR_SUBJ_REG(reg, sz)        LIR_create_subject(LIR_REGISTER,   reg,   -1,                  0,   NULL,           0,   sz)
#define LIR_SUBJ_CONST(val)          LIR_create_subject(LIR_CONSTVAL,   -1,    -1,                  0,   NULL,           val, CONF_get_full_bytness())
#define LIR_SUBJ_NUMBER(val, fl, sz) LIR_create_subject(LIR_NUMBER,     fl,    -1,                  0,   val,            0,   sz)
#define LIR_SUBJ_VAR(id, sz)         LIR_create_subject(LIR_VARIABLE,   -1,    id,                  -1,  NULL,           0,   sz)
#define LIR_SUBJ_GLVAR(id)           LIR_create_subject(LIR_GLVARIABLE, -1,    id,                  0,   NULL,           0,   0)
#define LIR_SUBJ_OFF(reg, off, sz)   LIR_create_subject(LIR_MEMORY,     reg,   -1,                  off, NULL,           0,   sz)
#define LIR_SUBJ_LABEL(id)           LIR_create_subject(LIR_LABEL,      -1,    id,                  0,   NULL,           0,   0)
#define LIR_SUBJ_RAWASM(l)           LIR_create_subject(LIR_RAWASM,     -1,    l,                   0,   NULL,           0,   0)
#define LIR_SUBJ_STRING(id)          LIR_create_subject(LIR_STRING,     -1,    id,                  0,   NULL,           0,   CONF_get_full_bytness())
#define LIR_SUBJ_FUNCNAME(n)         LIR_create_subject(LIR_FNAME,      -1,    n->storage.str.s_id, 0,   NULL,           0,   0)
#define LIR_SUBJ_ADDRFUNC(n)         LIR_create_subject(LIR_FNAME,      -1,    n->storage.str.s_id, 0,   NULL,           1,   CONF_get_full_bytness())
#define LIR_SUBJ_LIST()              LIR_create_subject(LIR_ARGLIST,    -1,    -1,                  0,   NULL,           0,   0)
#define LIR_SUBJ_LOCATION(tloc)      LIR_create_subject(LIR_FPOS,       -1,    -1,                  0,  (string_t*)tloc, 0,   0)

/* op */
#define LIR_BLOCK0(ctx, op) LIR_append_block(LIR_create_block((op), NULL, NULL, NULL), (ctx))
/* op (a) */
#define LIR_BLOCK1(ctx, op, fa) LIR_append_block(LIR_create_block((op), (fa), NULL, NULL), (ctx))
/* op (a), (b) */
#define LIR_BLOCK2(ctx, op, fa, sa) LIR_append_block(LIR_create_block((op), (fa), (sa), NULL), (ctx))
/* op (a), (b), (c) */
#define LIR_BLOCK3(ctx, op, fa, sa, ta) LIR_append_block(LIR_create_block((op), (fa), (sa), (ta)), (ctx))

#define CONCAT2(a,b) a##b
#define CONCAT(a,b)  CONCAT2(a,b)

#define iterate_lir_args(v, block, off)                                                     \
    lir_subject_t* CONCAT(__args_, __LINE__)[] = { block->farg, block->sarg, block->targ }; \
    for (int i = off; i < 3; i++)                                                           \
        for (                                                                               \
            v = CONCAT(__args_, __LINE__)[i];                                               \
            CONCAT(__args_, __LINE__)[i];                                                   \
            CONCAT(__args_, __LINE__)[i] = NULL                                             \
        )                                                                                   \
            if (CONCAT(__args_, __LINE__)[i])

#define iterate_ref_lir_args(v, block, off)                                                     \
    lir_subject_t** CONCAT(__args_, __LINE__)[] = { &block->farg, &block->sarg, &block->targ }; \
    for (int i = off; i < 3; i++)                                                               \
        for (                                                                                   \
            v = CONCAT(__args_, __LINE__)[i];                                                   \
            CONCAT(__args_, __LINE__)[i];                                                       \
            CONCAT(__args_, __LINE__)[i] = NULL                                                 \
        )                                                                                       \
            if (CONCAT(__args_, __LINE__)[i] && *CONCAT(__args_, __LINE__)[i])

#endif