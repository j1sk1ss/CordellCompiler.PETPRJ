#ifndef FUNC_TB_H_
#define FUNC_TB_H_

#include <std/str.h>
#include <std/map.h>
#include <std/list.h>
#include <prep/token_types.h>
#include <ast/ast.h>
#include <ast/dump.h>
#include <symtab/symtab_id.h>
#include <symtab/scopetb.h>

typedef struct {
    signed char global;   /* keyword   */
    signed char abi;      /* annot     */
    signed char weak;     /* annot     */
    signed char external; /* keyword   */
    signed char entry;    /* annot/sys */
    signed char used;     /* sys       */
    signed char local;    /* sys       */
    signed char vargs;    /* sys       */
    signed char generic;  /* sys       */
    signed char self;     /* annot     */
    signed char naked;    /* 1 / 0   */ /* annot */
    signed char inln;     /* 1, 2, 3 */ /* annot */
    signed char onlybody; /* annot     */
    signed char vname;    /* annot     */
    signed char abstract; /* annot     */
    signed char override; /* annot     */
} func_info_flags_t;

typedef struct {
    symbol_id_t       id;               /* ID in the symtable                       */
    symbol_id_t       s_id;

    string_t*         name;             /* Base function name                       */
    string_t*         virt;             /* De-virtual name                          */
    func_info_flags_t flags;
    
    list_t            local;            /* Local functions                          */
    ast_node_t*       args;             /* Input arguments                          */
    ast_node_t*       rtype;            /* Function return type                     */
    struct {
        list_t        registered_types; /* List of registered symbol_id_t types     */
        map_t         generic;          /* Generic base types. TypeId <-> TokenType */
        list_t        resolutions;      /* Resolved copies (use the generic field)  */
    } template;
} func_info_t;

typedef struct func_ctx {
    symbol_id_t curr_id;
    map_t       functb;
} functab_ctx_t;

/*
Collect all functions with the same name.
Params:
    - `fname` - Target function name.
    - `s_id` - Function call scope.
    - `out` - The output list.
    - `ctx` - Function symbol table.
    - `sctx` - Scope symbol table.

Returns 1 on success, otherwise 0.
*/
int FNTB_collect_info(string_t* fname, symbol_id_t s_id, list_t* out, functab_ctx_t* ctx, scopetab_ctx_t* sctx);

/*
Get function from a table by the provided ID.
Params:
    - `id` - Function's ID.
    - `out` - Function output body.
    - `ctx` - Function symbol table.

Returns 1 on success, otherwise 0.
*/
int FNTB_get_info_id(symbol_id_t id, func_info_t* out, functab_ctx_t* ctx);

/*
Get function from a table by the provided name.
Params:
    - `fname` - Function's name.
    - `sid` - Scope ID.
    - `out` - Function output body.
    - `ctx` - Function symbol table.

Returns 1 on success, otherwise 0.
*/
int FNTB_get_info(string_t* fname, symbol_id_t s_id, func_info_t* out, functab_ctx_t* ctx);

/*
Add a new function to a function symbol table.
Params:
    - `name` - Function's name.
               Note: Will copy the provided name.
    - `vname` - Vartual function's name.
                Note: May be the 'NULL' value.
    - `flags` - Function's flags.
    - `args` - Function's arguments from AST.
    - `rtype` - Function's return type from AST.
    - `ctx` - Function symbol table.

Returns -1 if fails or a new function's ID.
*/
symbol_id_t FNTB_add_info(
    string_t* name, string_t* vname, func_info_flags_t flags, symbol_id_t s_id, ast_node_t* args, ast_node_t* rtype, functab_ctx_t* ctx
);

/*
Create a copy of a function. Will copy all fields except locals.
Params:
    - `src` - Function to copy.
    - `ctx` - Symtable context.

Returns the ID of a copied function.
*/
symbol_id_t FNTB_add_copy(func_info_t* src, functab_ctx_t* ctx);

/*
Check whether a function has registered generic types.
Params:
    - `f_id` - Function ID.
    - `ctx` - Symtable context.

Returns 1 if generic types are registered, otherwise 0.
*/
int FNTB_has_generic_types(symbol_id_t f_id, functab_ctx_t* ctx);

/*
Reset registered types for a function. Can be useful in the situation
when you re-register a function from a prototype to an implementation.
Params:
    - `f_id` - Function ID.
    - `ctx` - Symtable context.

Returns 1 if succeeds.
*/
int FNTB_clear_generic_types(symbol_id_t f_id, functab_ctx_t* ctx);

/*
Add a generic type for a function. This function adds a type obly if it is
a generic type. Consider to add the type to a symtable first.
Params:
    - `f_id` - Function ID.
    - `t_id` - Type ID.
    - `ctx` - Symtable context.

Returns 1 if succeeds.
*/
int FNTB_register_generic_type(symbol_id_t f_id, symbol_id_t t_id, functab_ctx_t* ctx);

/*
Register an existed function as a local function.
Params:
    - `f_id` - Parent function.
    - `l_id` - Local function.
    - `ctx` - Function symbol table.

Returns 1 on success, otherwise 0.
*/
int FNTB_add_local(symbol_id_t f_id, symbol_id_t l_id, functab_ctx_t* ctx);

#define FNTB_ONLY_NAME(name)   name, FNTB_NO_FLAGS_CHANGE, NULL, NULL
#define FNTB_ONLY_FLAGS(flags) NULL, flags, NULL, NULL
#define FNTB_SHALLOW_EXTERN  2
#define FNTB_EXPLICIT_EXTERN 1
#define FNTB_NO_EXTERN       0
#define FNTB_NO_FLAGS_CHANGE ((func_info_flags_t){ .global=-1, .abi=-1, .weak=-1, .external=-1, .entry=-1, .used=-1,  \
    .local=-1, .vargs=-1, .generic=-1, .self=-1, .naked=-1, .inln=-1 })
#define FNTB_SET_EXTERNAL(n) ((func_info_flags_t){ .global=-1, .abi=-1, .weak=-1, .external=n, .entry=-1, .used=-1,   \
    .local=-1, .vargs=-1, .generic=-1, .self=-1, .naked=-1, .inln=-1 })
#define FNTB_SET_NAKED(n)    ((func_info_flags_t){ .global=-1, .abi=-1, .weak=-1, .external=-1, .entry=-1, .used=-1,  \
    .local=-1, .vargs=-1, .generic=-1, .self=-1, .naked=(n), .inln=-1 })
#define FNTB_SET_GENERIC(n)  ((func_info_flags_t){ .global=-1, .abi=-1, .weak=-1, .external=-1, .entry=-1, .used=-1,  \
    .local=-1, .vargs=-1, .generic=(n), .self=-1, .naked=-1, .inln=-1 })
#define FNTB_SET_USED(n)     ((func_info_flags_t){ .global=-1, .abi=-1, .weak=-1, .external=-1, .entry=-1, .used=(n), \
    .local=-1, .vargs=-1, .generic=-1, .self=-1, .naked=-1, .inln=-1 })
/*
Update an existed function.
Note: Will update the virtual name of a function.
Params:
    - `id` - Function ID.
    - `name` - New name.
    - `used` - Is this function used?
    - `external` - Is this function external?
    - `global` - Is this function global?
    - `local` - Is this a local function?
    - `entry` - Is this an entry function?
    - `naked` - Is this a naked function?
    - `vargs` - Is this function has the vargs arg?
    - `args` - Function's arguments from AST.
    - `rtype` - Function's return type from AST.
    - `ctx` - Function symtable context.

Returns 1 on success, otherwise 0.
*/
int FNTB_update_func(
    symbol_id_t id, string_t* name, func_info_flags_t flags, ast_node_t* args, ast_node_t* rtype, functab_ctx_t* ctx
);

/*
Create a copy with resolved generic types. Will create a copy with a modified name. Consider
to check whether a function with the provided types already exists.
Params:
    - `id` - Source founction (template function).
    - `types` - List of provided token types.
    - `ctx` - Symtable context.

Returns the ID of a new function.
*/
symbol_id_t FNTB_create_resolved_copy(symbol_id_t id, list_t* types, functab_ctx_t* ctx);

/*
Unload a function symtable context.
Params:
    - `ctx` - Function symtable context.

Returns 1 if succeeds.
*/
int FNTB_unload(functab_ctx_t* ctx);

#define fn_iterate_args(fn) \
    for (ast_node_t* arg = (fn)->args->c; arg && arg->t->t_type != SCOPE_TOKEN; arg = arg->siblings.n)

#endif
