#ifndef ANNOT_H_
#define ANNOT_H_

#include <std/mm.h>
#include <std/str.h>
#include <std/stack.h>
#include <symtab/symtab_id.h>

#define ENTRY_ANNOTATION_COMMAND "entry"
#define ALIGN_ANNOTATION_COMMAND "align"
#define LIKEC_ANNOTATION_COMMAND "like_c"
#define UNION_ANNOTATION_COMMAND "union"
#define WEAKS_ANNOTATION_COMMAND "weak"
#define ABICC_ANNOTATION_COMMAND "abi"
#define NAKED_ANNOTATION_COMMAND "naked"
#define SECTN_ANNOTATION_COMMAND "section"
#define NOSEC_ANNOTATION_COMMAND "nosection"
#define BODYO_ANNOTATION_COMMAND "only_body"
#define NOFAL_ANNOTATION_COMMAND "no_fall"
#define NTLAZ_ANNOTATION_COMMAND "not_lazy"
#define STRGH_ANNOTATION_COMMAND "straight"
#define COUNT_ANNOTATION_COMMAND "counter"
#define HOTSC_ANNOTATION_COMMAND "hot"
#define COLDS_ANNOTATION_COMMAND "cold"
#define REGST_ANNOTATION_COMMAND "register"
#define POPRG_ANNOTATION_COMMAND "poparg"
#define PPREG_ANNOTATION_COOMAND "popreg"
#define SSELF_ANNOTATION_COMMAND "self"
#define VNAME_ANNOTATION_COMMAND "vname"
#define NNULL_ANNOTATION_COMMAND "not_null"
#define VOLAT_ANNOTATION_COMMAND "volatile"
#define VTABL_ANNOTATION_COMMAND "vtable"
#define ABSTR_ANNOTATION_COMMAND "abstract"
#define OVERD_ANNOTATION_COMMAND "override"

#define INLNE_ANNOTATION_COMMAND "inline" /* inline / inline(always) / inline(never) */
#define INLNE_YES_OPTION         "always"
#define INLNE_NO_OPTION          "never"
#define INLINE_MODEL_OPTION      "model"
#define ALWAYS_INLINE            1
#define NEVER_INLINE             2
#define SOFT_YES_INLINE          3
#define MODEL_INLINE             4

typedef enum {
    ANNOTATION_VALUE_PARAM,
    ANNOTATION_VARIABLE_PARAM
} annotation_param_type_t;

typedef struct {
    string_t*               string;
    union {
        long                value;
        symbol_id_t         v_id;
    };
    annotation_param_type_t t;
    char                    filled : 1;
} annotation_param_t;

typedef struct {
    union {
        long                value;
        symbol_id_t         v_id;
    } index;
    annotation_param_type_t idx_t;
    union {
        long                value;
        symbol_id_t         v_id;
    } step;
    annotation_param_type_t stp_t;
    char                    has_index : 1;
    char                    has_step  : 1;
} annotation_counter_t;

typedef struct {
    string_t*            section;
    int                  salign;    /* Section's align */
    string_t*            fname;
    int                  do_inline; /* 1 - strongly yes, 2 - strongly no, 3 - increase inline chance */
    int                  align;
    annotation_counter_t counter;
    short                reg;
    short                pop_register;
    char                 is_vname    : 1;
    char                 is_nosec    : 1;
    char                 is_naked    : 1;
    char                 is_entry    : 1;
    char                 is_nofall   : 1;
    char                 is_notlazy  : 1;
    char                 is_straight : 1;
    char                 is_hot      : 1;
    char                 is_cold     : 1;
    char                 is_argpop   : 1;
    char                 is_self     : 1;
    char                 is_like_c   : 1;
    char                 is_union    : 1;
    char                 is_weak     : 1;
    char                 is_abi      : 1;
    char                 is_onlybody : 1;
    char                 is_notnull  : 1;
    char                 is_volatile : 1;
    char                 is_vtable   : 1;
    char                 is_abstract : 1;
    char                 is_override : 1;
} annotations_summary_t;

typedef enum {
    UNKNOWN_ANNOTATION,
    ALIGN_ANNOTATION,     /* Set the align of a declaration                 */
    SECTION_ANNOTATION,   /* Put a declration or function to a section      */
    NOSECTION_ANNOTATION, /* Put a declaration or function out a sec        */
    NAKED_ANNOTATION,     /* Don't unpack START, FDECL                      */
    ENTRY_ANNOTATION,     /* Is this an entry function?                     */
    NOFALL_ANNOTATION,    /* switch with a break as a default command       */
    NOTLAZY_ANNOTATION,   /* && and || with full evaluation                 */
    STRAIGHT_ANNOTATION,  /* switch based on if-elseif-else                 */
    COUNTER_ANNOTATION,   /* hidden counter-break instructure               */
    HOT_ANNOTATION,       /* Will make the linked else branch cold          */
    COLD_ANNOTATION,      /* Will make the linked then branch hot           */
    REGISTER_ANNOTATION,  /* Will link the selected register to a decl      */
    POPARG_ANNOTATION,    /* Will pop value from the stack to a linked      */
    POPREG_ANNOTATION,    /* Will load value from a specific register       */
    INLINE_ANNOTATION,    /* Will change inline decider result              */
    SELF_ANNOTATION,      /* Will tell devirt that a function is static     */
    LIKEC_ANNOTATION,     /* Will tell container to generate C offsets      */
    UNION_ANNOTATION,     /* Will tell container to store fields union      */
    WEAK_ANNOTATION,      /* Will mark a function as weak                   */
    ABI_ANNOTATION,       /* Will mark a function as ABI-compatible         */
    ONLYBODY_ANNOTATION,  /* Will say that the function is just a container */
    VNAME_ANNOTATION,     /* Will set a vartial name for a function         */
    NOTNULL_ANNOTATION,   /* Will mark a variable as a not Null variable    */
    VOLATILE_ANNOTATION,  /* Will mark variable as a important variable     */
    VTABLE_ANNOTATION,    /* Will enable vtable in a container              */
    ABSTRACT_ANNOTATION,  /* Will mark a method as an abstract method       */
    OVERRIDE_ANNOTAITON,  /* Will mark function as an override for somebody */
} annotation_type_t;

typedef struct {
    annotation_type_t        t;
    union {
        int                  align;      /* ALIGN_ANNOTATION     */
        string_t*            fname;      /* ENTRY_ANNOTATION     */
        struct {
            string_t*        section;
            int              align;
        } section;                       /* SECTION_ANNOTATION   */
        string_t*            inline_opt; /* INLINE_ANNOTATION    */
        annotation_counter_t counter;    /* COUNTER_ANNOTATION   */
        short                regval;     /* REGISTER_ANNOTATION  */
    } data;
} annotation_t;

int           ANNOT_read_annotations(sstack_t* annots, annotations_summary_t* summary);
int           ANNOT_destroy_summary(annotations_summary_t* summray);
annotation_t* ANNOT_create_annotation(annotation_type_t t, annotation_param_t* fp, annotation_param_t* sp);
int           ANNOT_destroy_annotation(annotation_t* annot);

#endif
