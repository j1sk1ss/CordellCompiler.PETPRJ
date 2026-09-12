#ifndef HIRGENS_H_
#define HIRGENS_H_

#include <std/math.h>
#include <std/qsort.h>
#include <ast/ast.h>
#include <ast/astgen.h>
#include <ast/astgen/annot.h>
#include <ast/devirt.h>
#include <hir/hir.h>
#include <hir/hir_types.h>

hir_subject_t* HIR_add_to_subject(hir_subject_t* src, sym_table_t* smt, long add, hir_ctx_t* ctx);
hir_subject_t* HIR_gdref_subject(hir_subject_t* src, sym_table_t* smt, hir_ctx_t* ctx);

/* Check if node has an annotation.
   Params:
        - `t` - Target annotation Type.
        - `nd` - Source node.
        - `act` - Action if the annotation found */
#define HAS_ANNOTATION(type, nd, act)            \
    foreach (annotation_t* annot, &nd->annots) { \
        if (annot->t == type) { act; break; }    \
    }

/* Dump and load information for the 'poparg' keyword.
Params:
    - `op` - poparg operation
    - `args` - Current popped arguments number.
    - `logic` - Wrapped logic */
#define SET_AND_DUMP_POPARG(r, vargs, logic) \
    void *prtype = ctx->carry.rtype,         \
         *pvargs = ctx->carry.varg;          \
    ctx->carry.rtype = r;                    \
    ctx->carry.varg = vargs;                 \
    logic;                                   \
    ctx->carry.rtype = prtype;               \
    ctx->carry.varg = pvargs;

/* Fire a HIRGEN error.
Params:
    - `ctx` - HIR ctx.
    - `msg` - Message to fire */
#define HIRGEN_ERROR(ctx, msg, ...)                   \
    fprintf(                                          \
        stderr,                                       \
        "[%s:%li:%li] " msg "\n",                     \
        ctx->pos.file ? ctx->pos.file->body : "base", \
        ctx->pos.line,                                \
        ctx->pos.column,                              \
        ##__VA_ARGS__                                 \
    )

int HIR_generate_position(file_position_t* pos, hir_ctx_t* ctx);
#define HIR_SET_CURRENT_POS(ctx, nd) HIR_generate_position(nd->t ? &nd->t->finfo : NULL, ctx);

/*
Generate HIR loads for function arguments and declare them in the current scope.
Params:
    - `args` - Function argument list AST node.
    - `ctx` - HIR ctx.
    - `fi` - Function metadata.

Returns the first AST node after the argument list, usually the function body.
*/
ast_node_t* HIR_generate_argument_load(ast_node_t* args, hir_ctx_t* ctx, func_info_t* fi);

/*
Generate implict convertion from the one type to another. 
Note: If the types are similar, it doesn't perform the convertation proccess.
Params:
    - `ctx` - HIR ctx.
    - `ptr` - Target reference level.
    - `t` - Target type.
    - `src` - Source HIR subject.
    - `smt` - Symtable.

Return converted HIR subject.
*/
hir_subject_t* HIR_generate_implconv(hir_ctx_t* ctx, char ptr, hir_subject_type_t t, hir_subject_t* src, sym_table_t* smt);

/*
Generate explict convertion from the one type to another. 
Note: If the types are similar, it doesn't perform the convertation proccess.
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return converted HIR subject.
*/
hir_subject_t* HIR_generate_explconv(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert AST node into HIR element.
Note: In comparision the the HIR_generate_block, this function
      doesn't care about the entier scope and the further code.
      It will convert only the provided node and their children.
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_elem(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert funccall AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.
    - `ret` - Will return or not?

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_funccall(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt, int ret);

/*
Convert load AST node (arr[i]) into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_load(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert oprand AST node (x + x) into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_operand(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.
    - `ret` - Will return or not?

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_syscall(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt, int ret);

/*
Convert a breakpoint AST node into a HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_breakpoint_block(ast_node_t* node, hir_ctx_t* ctx);

/*
Convert a break AST node into a HIR element. 
Note: ctx->carry must be a non-NULL value!
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_break_block(ast_node_t* node, hir_ctx_t* ctx);

/*
Convert extern AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_extern_block(ast_node_t* node, hir_ctx_t* ctx);

/*
Convert operation (+=, -=, *=, /=) AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.
    - `ret` - If this is a block, must be '0'.

Returns the 'NULL' value or an update operator.
*/
hir_subject_t* HIR_generate_update_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt, int ret);

/*
Convert load AST node (arr[i] = 0) node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_store_block(ast_node_t* node, hir_subject_t* src, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert asmblock AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_asmblock(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert assignment AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_assignment_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert block AST (from SCOPE to SCOPE) node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert if AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_if_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert while AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_while_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert loop AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_loop_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert switch AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_switch_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert declaration AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_declaration_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert return AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_return_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert function AST node into HIR element. 
Params:
    - `node` - AST node.
    - `f_id` - Provide ID if you want to overwrite the node's ID.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_function_block(ast_node_t* node, symbol_id_t f_id, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert start AST node into HIR element. 
Params:
    - `node - AST node.
    - `ctx - HIR ctx.
    - `smt - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_start_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert exit AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_exit_block(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert unary AST node into a HIR operation.
Params:
    - `node` - Unary AST node.
    - `ctx` - HIR ctx.
    - `op` - HIR unary operation to emit.
    - `smt` - Symtable.

Return generated HIR subject.
*/
hir_subject_t* HIR_generate_unary(ast_node_t* node, hir_ctx_t* ctx, hir_operation_t op, sym_table_t* smt);

/*
Create referenced subject from the source. 
Params:
    - `src` - Source HIR subject.
    - `smt` - Symtable.
    - `inc` - Increment reference counter.
              Note: By default set to 1. If set to 0 - 
                    won't increment the `ptr` field.

Return referenced subject.
*/
hir_subject_t* HIR_reference_subject(hir_subject_t* src, sym_table_t* smt, int inc);

/*
Convert ref AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_ref(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert dref AST node into HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.
    - `data` - By default is NULL.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_dref(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt, hir_subject_t* data);

/*
Convert indexation AST node into HIR element. 
Snippet:
```cpl
something = array[index]
```

Params:
    - `node` - Indexation node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return parsed from AST HIR subject.
*/
hir_subject_t* HIR_generate_load_indexation(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);
hir_subject_t* HIR_generate_ref_indexation(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert indexation AST node into HIR element. 
Snippet:
```cpl
array[index] = something
```

Params:
    - `node` - Indexation node.
    - `data` - Information to store.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 on success, otherwise 0.
*/
int HIR_generate_store_indexation(ast_node_t* node, hir_subject_t* data, hir_ctx_t* ctx, sym_table_t* smt);

/*
Will generate a CONST subject that represents the size of the input subject.
Params:
    - `s` - The target subject.
    - `smt` - Symtable.

Returns a Constant subject, which represents the size of the subject.
*/
hir_subject_t* HIR_generate_sizeof(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Convert lambda AST node to a HIR element. 
Params:
    - `node` - AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.
    - `ret` - If this is a block, must be '0'.

Returns the 'NULL' value or an update operator.
*/
hir_subject_t* HIR_generate_lambda(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt, int ret);

/*
Convert member access AST node into a HIR load.
Params:
    - `node` - Member access AST node.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Return loaded member value.
*/
hir_subject_t* HIR_generate_load_member_access(ast_node_t* node, hir_ctx_t* ctx, sym_table_t* smt);

/*
Find variable metadata for a concrete container field type.
Uses the field owner and field name when the type belongs to a container,
falling back to a type-only lookup for non-field types.
*/
int HIR_find_member_variable(type_info_t* field_info, symbol_id_t owner_id, string_t* name, variable_info_t* var_info, sym_table_t* smt);

/*
Syntheticly move a head towards the field (by sub-type Id).
Params:
    - `root` - Container field access node.
    - `ctx` - HIR ctx.
    - `field_info` - Ouput field info. Will fill the structure
                     if it will find the field.
    - `smt` - Symtable.

Returns a pointer to the field. 
*/
hir_subject_t* HIR_point_to_field(ast_node_t* root, hir_ctx_t* ctx, type_info_t* field_info, sym_table_t* smt);

/* 
Load the pointer stored in an array field header.
Array fields keep the element buffer head separately from the container field.
Params:
    - `head` - Address of the array field header.
    - `ai` - Array metadata.
    - `ctx` - HIR context.
    - `smt` - Symtable.

Returns a temporary subject that points to the first array element. 
*/
hir_subject_t* HIR_load_array_field_head(hir_subject_t* head, array_info_t* ai, hir_ctx_t* ctx, sym_table_t* smt);

/*
Store data to a filed of a container.
Params:
    - `node` - Store operation node.
    - `data` - The data which is stored in the field.
    - `ctx` - HIR ctx.
    - `smt` - Symtable.

Returns 1 if there is no errors.
*/
int HIR_generate_store_member_access(ast_node_t* node, hir_subject_t* data, hir_ctx_t* ctx, sym_table_t* smt);

#endif
