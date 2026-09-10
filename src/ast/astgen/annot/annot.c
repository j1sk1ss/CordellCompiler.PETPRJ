#include <ast/astgen/annot.h>

annotation_t* ANNOT_create_annotation(annotation_type_t t, annotation_param_t* fp, annotation_param_t* sp) {
    annotation_t* annot = (annotation_t*)mm_malloc(sizeof(annotation_t));
    if (!annot) return NULL;
    str_memset(annot, 0, sizeof(annotation_t));
    annot->t = t;
    switch (t) {
        case ALIGN_ANNOTATION:     annot->data.align = (int)fp->value;                                    break;
        case REGISTER_ANNOTATION:  annot->data.regval = (short)fp->value;                                 break;
        case POPREG_ANNOTATION:    annot->data.regval = (short)fp->value;                                 break;
        case VNAME_ANNOTATION:
        case ENTRY_ANNOTATION:     if (fp->string) annot->data.fname = fp->string->copy(fp->string);      break;
        case INLINE_ANNOTATION:    if (fp->string) annot->data.inline_opt = fp->string->copy(fp->string); break;
        case COUNTER_ANNOTATION: {
            if (fp && fp->filled) {
                annot->data.counter.has_index = 1;
                annot->data.counter.idx_t     = fp->t;
                switch (fp->t) {
                    case ANNOTATION_VARIABLE_PARAM: annot->data.counter.index.v_id  = fp->v_id;           break;
                    case ANNOTATION_VALUE_PARAM:    annot->data.counter.index.value = fp->value;          break;
                    default:                                                                              break;
                }
            }

            if (sp && sp->filled) {
                annot->data.counter.has_step = 1;
                annot->data.counter.stp_t    = sp->t;
                switch (sp->t) {
                    case ANNOTATION_VARIABLE_PARAM: annot->data.counter.step.v_id  = sp->v_id;            break;
                    case ANNOTATION_VALUE_PARAM:    annot->data.counter.step.value = sp->value;           break;
                    default:                                                                              break;
                }
            }

            break;
        }
        case SECTION_ANNOTATION: {
            annot->data.section.align = SMT_NULL;
            if (fp->filled) annot->data.section.section = fp->string->copy(fp->string);
            if (sp->filled) annot->data.section.align   = (int)sp->value;
            break;
        }
        default: break;
    }

    return annot;
}

int ANNOT_read_annotations(sstack_t* annots, annotations_summary_t* summary) {
    annotation_t* annot;
    while (stack_pop(annots, (void**)&annot)) {
        switch (annot->t) {
            case INLINE_ANNOTATION: {
                if (!annot->data.inline_opt) summary->do_inline = SOFT_YES_INLINE;
                else {
                    if (annot->data.inline_opt->requals(annot->data.inline_opt, INLNE_YES_OPTION))         summary->do_inline = ALWAYS_INLINE;
                    else if (annot->data.inline_opt->requals(annot->data.inline_opt, INLNE_NO_OPTION))     summary->do_inline = NEVER_INLINE;
                    else if (annot->data.inline_opt->requals(annot->data.inline_opt, INLINE_MODEL_OPTION)) summary->do_inline = MODEL_INLINE;
                }

                break;
            }
            case SECTION_ANNOTATION: {
                if (summary->section) destroy_string(summary->section);
                summary->section = annot->data.section.section->copy(annot->data.section.section); 
                summary->salign  = annot->data.section.align;
                break;
            }
            case VNAME_ANNOTATION: summary->is_vname = 1; goto _set_vname;
            case ENTRY_ANNOTATION: {
                summary->is_entry = 1;
_set_vname: {}
                if (summary->fname) destroy_string(summary->fname);
                summary->fname = annot->data.fname ? annot->data.fname->copy(annot->data.fname) : NULL;
                break;
            }
            case NOSECTION_ANNOTATION: summary->is_nosec     = 1;                   break;
            case ALIGN_ANNOTATION:     summary->align        = annot->data.align;   break;
            case NAKED_ANNOTATION:     summary->is_naked     = 1;                   break;
            case NOFALL_ANNOTATION:    summary->is_nofall    = 1;                   break;
            case NOTLAZY_ANNOTATION:   summary->is_notlazy   = 1;                   break;
            case STRAIGHT_ANNOTATION:  summary->is_straight  = 1;                   break;
            case HOT_ANNOTATION:       summary->is_hot       = 1;                   break;
            case COLD_ANNOTATION:      summary->is_cold      = 1;                   break;
            case POPARG_ANNOTATION:    summary->is_argpop    = 1;                   break;
            case POPREG_ANNOTATION:    summary->pop_register = annot->data.regval;  break;
            case SELF_ANNOTATION:      summary->is_self      = 1;                   break;
            case LIKEC_ANNOTATION:     summary->is_like_c    = 1;                   break;
            case UNION_ANNOTATION:     summary->is_union     = 1;                   break;
            case WEAK_ANNOTATION:      summary->is_weak      = 1;                   break;
            case ABI_ANNOTATION:       summary->is_abi       = 1;                   break;
            case ONLYBODY_ANNOTATION:  summary->is_onlybody  = 1;                   break;
            case NOTNULL_ANNOTATION:   summary->is_notnull   = 1;                   break;
            case VOLATILE_ANNOTATION:  summary->is_volatile  = 1;                   break;
            case VTABLE_ANNOTATION:    summary->is_vtable    = 1;                   break;
            case REGISTER_ANNOTATION:  summary->reg          = annot->data.regval;  break;
            case COUNTER_ANNOTATION:   summary->counter      = annot->data.counter; break;
            default: break;
        }

        ANNOT_destroy_annotation(annot);
    }

    return 1;
}

int ANNOT_destroy_summary(annotations_summary_t* summray) {
    if (summray->section) destroy_string(summray->section);
    if (summray->fname)   destroy_string(summray->fname);
    return 1;
}

int ANNOT_destroy_annotation(annotation_t* annot) {
    switch (annot->t) {
        case SECTION_ANNOTATION: destroy_string(annot->data.section.section);                        break;
        case VNAME_ANNOTATION:
        case ENTRY_ANNOTATION:   if (annot->data.fname) destroy_string(annot->data.fname);           break;
        case INLINE_ANNOTATION:  if (annot->data.inline_opt) destroy_string(annot->data.inline_opt); break;
        default: break;
    }

    mm_free(annot);
    return 1;
}
