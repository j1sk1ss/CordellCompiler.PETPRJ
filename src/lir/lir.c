#include <lir/lir.h>

static unsigned long _curr_id = 0;

lir_subject_t* LIR_create_subject(lir_subject_type_t t, int reg, int v_id, long offset, string_t* strval, long intval, int size) {
    lir_subject_t* subj = mm_malloc(sizeof(lir_subject_t));
    if (!subj) return NULL;
    str_memset(subj, 0, sizeof(lir_subject_t));

    subj->t    = t;
    subj->size = (char)size;
    subj->id   = _curr_id++;

    switch (t) {
        case LIR_ARGLIST:  list_init(&subj->storage.list.h);                       break;
        case LIR_REGISTER: subj->storage.reg.reg = LIR_format_register(reg, size); break;
        case LIR_VARIABLE:
        case LIR_GLVARIABLE:
        case LIR_STVARIABLE: 
            subj->storage.var.v_id = v_id; goto _variable_complete;
        case LIR_MEMORY: {
            subj->storage.var.base = reg;
_variable_complete: {}
            subj->storage.var.offset = offset;
            break;
        }
        case LIR_LABEL:    subj->storage.lb.lb_id   = v_id;   break;
        case LIR_CONSTVAL: subj->storage.cnst.value = intval; break;
        case LIR_FNAME:
        case LIR_RAWASM:
        case LIR_STRING: {
            subj->storage.str.sid = v_id; 
            subj->storage.str.rel = intval;
            break;
        }
        CONDITIONAL_CASE(LIR_NUMBER, strval) { /* reg is used here as a flag which shows whether the number is float or not */
            subj->storage.num.is_float = reg ? 1 : 0;
            subj->storage.num.value = strval->copy(strval);
            break;
        }
        CONDITIONAL_CASE(LIR_FPOS, strval) {
            str_memcpy(&subj->storage.pos, strval, sizeof(file_position_t));
        }
        default: break;
    }

    return subj;
}

lir_subject_t* LIR_copy_subject(lir_subject_t* s) {
    lir_subject_t* subj = mm_malloc(sizeof(lir_subject_t));
    if (!subj) return NULL;
    str_memset(subj, 0, sizeof(lir_subject_t));

    subj->t     = s->t;
    subj->size  = s->size;
    subj->dsize = s->dsize;
    subj->id    = _curr_id++;

    switch (s->t) {
        case LIR_ARGLIST: {
            list_init(&subj->storage.list.h);
            lir_subject_t* arg;
            foreach (arg, &s->storage.list.h) {
                list_add(&subj->storage.list.h, LIR_copy_subject(arg));
            }

            break;
        }
        case LIR_FPOS:       case LIR_LABEL:    case LIR_FNAME:
        case LIR_RAWASM:     case LIR_MEMORY:   case LIR_STRING:
        case LIR_CONSTVAL:   case LIR_REGISTER: case LIR_VARIABLE:
        case LIR_GLVARIABLE: case LIR_STVARIABLE: {
            str_memcpy(&subj->storage, &s->storage, sizeof(s->storage));
            break;
        }
        CONDITIONAL_CASE(LIR_NUMBER, s->storage.num.value) {
            subj->storage.num.is_float = s->storage.num.is_float;
            subj->storage.num.value    = s->storage.num.value->copy(s->storage.num.value);
            break;
        }
        default: break;
    }

    return subj;
}

lir_block_t* LIR_create_block(lir_operation_t op, lir_subject_t* fa, lir_subject_t* sa, lir_subject_t* ta) {
    lir_block_t* blk = mm_malloc(sizeof(lir_block_t));
    if (!blk) return NULL;

    blk->op     = op;
    blk->farg   = fa;
    blk->sarg   = sa;
    blk->targ   = ta;
    blk->next   = blk->prev = NULL;
    blk->unused = 0;

    if (fa && !fa->home) fa->home = blk;
    if (sa && !sa->home) sa->home = blk;
    if (ta && !ta->home) ta->home = blk;

    return blk;
}

int LIR_subj_equals(lir_subject_t* a, lir_subject_t* b) {
    if (!a || !b || a->t != b->t) return 0;
    if (a == b) return 1;
    switch (a->t) {
        case LIR_CONSTVAL:   return a->storage.cnst.value == b->storage.cnst.value;
        case LIR_LABEL:      return a->storage.lb.lb_id == b->storage.lb.lb_id;
        case LIR_RAWASM:
        case LIR_FNAME:
        case LIR_STRING:     return a->storage.str.sid == b->storage.str.sid &&
                                    a->storage.str.rel == b->storage.str.rel;
        case LIR_VARIABLE:   return a->storage.var.v_id == b->storage.var.v_id;
        case LIR_MEMORY:     return (a->size == b->size) && 
                                    (a->storage.var.offset == b->storage.var.offset) && 
                                    (a->storage.var.base == b->storage.var.base);
        case LIR_GLVARIABLE:
        case LIR_STVARIABLE: return (a->storage.var.offset == b->storage.var.offset) && 
                                    (a->storage.var.v_id == b->storage.var.v_id);
        case LIR_REGISTER:   return LIR_format_register(a->storage.reg.reg, 1) == LIR_format_register(b->storage.reg.reg, 1);
        case LIR_NUMBER: {
            if (a->storage.num.is_float != b->storage.num.is_float) return 0;
            if (!a->storage.num.value || !b->storage.num.value) return a->storage.num.value == b->storage.num.value;
            return a->storage.num.value->equals(a->storage.num.value, b->storage.num.value);
        }
        case LIR_FPOS:       return str_memcmp(&a->storage.pos, &b->storage.pos, sizeof(file_position_t));
        case LIR_ARGLIST: {
            if (a->storage.list.h.s != b->storage.list.h.s) return 0;
            list_node_t* an = a->storage.list.h.h;
            list_node_t* bn = b->storage.list.h.h;
            while (an && bn) {
                if (!LIR_subj_equals((lir_subject_t*)an->data, (lir_subject_t*)bn->data)) return 0;
                an = an->n;
                bn = bn->n;
            }
            
            return !an && !bn;
        }
        default:             return 0;
    }
}

int LIR_append_block(lir_block_t* block, lir_ctx_t* ctx) {
    if (!ctx || !block) return -1;
    if (!ctx->h) ctx->h = ctx->t = block;
    else {
        block->prev  = ctx->t;
        ctx->t->next = block;
        ctx->t       = block;
    }
    
    return 0;
}

int LIR_insert_block_after(lir_block_t* block, lir_block_t* pos) {
    if (!block || !pos) return 0;
    block->prev = pos;
    block->next = pos->next;
    if (pos->next) pos->next->prev = block;
    pos->next = block;
    return 1;
}

int LIR_insert_block_before(lir_block_t* block, lir_block_t* pos) {
    if (!block || !pos) return 0;
    block->prev = pos->prev;
    block->next = pos;
    if (pos->prev) pos->prev->next = block;
    pos->prev = block;
    return 1;
}

int LIR_unlink_block(lir_block_t* block) {
    if (!block) return 0;
    if (block->prev) block->prev->next = block->next;
    if (block->next) block->next->prev = block->prev;
    block->prev = block->next = NULL;
    return 1;
}

int LIR_unload_subject(lir_subject_t* s) {
    if (!s) return 0;
    switch (s->t) {
        case LIR_NUMBER:  destroy_string(s->storage.num.value);                                       break;
        case LIR_ARGLIST: list_free_force_op(&s->storage.list.h, (int (*)(void*))LIR_unload_subject); break;
        default: break;
    }
    
    return mm_free(s);
}

int LIR_unload_blocks(lir_block_t* block) {
    if (!block) return -1;
    while (block) {
        lir_block_t* nxt = block->next;
        if (block->farg) LIR_unload_subject(block->farg);
        if (block->sarg) LIR_unload_subject(block->sarg);
        if (block->targ) LIR_unload_subject(block->targ);
        mm_free(block);
        block = nxt;
    }

    return 0;
}
