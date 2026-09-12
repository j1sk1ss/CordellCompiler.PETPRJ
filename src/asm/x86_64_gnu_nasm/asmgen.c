#include <asm/x86_64_gnu_nasm_asmgen.h>

/* Convert one LIR block into x86_64 GNU NASM assembly and write it to output.
Params:
    - `b` - LIR block to emit.
    - `fi` - Current function info for prologue/epilogue generation.
    - `smt` - Symtable used to resolve symbols.
    - `output` - Output assembly stream.

Returns 1 if succeeds. */
static int _convert_lirblock_to_assembly(lir_block_t* b, func_info_t* fi, sym_table_t* smt, FILE* output) {
    if (b->unused) return 1;
    switch (b->op) {
        case LIR_SETPOS: {
            if (
                !CONF_is_debug_compilation() ||
                !b->farg->storage.pos.file
            ) break;
            EMIT_COMMAND("%%line %li \"%s\"", b->farg->storage.pos.line, b->farg->storage.pos.file->body);
            break;
        }
        case LIR_FCLL:
        case LIR_ECLL: {
            func_info_t fi;
            if (FNTB_get_info_id(b->farg->storage.str.sid, &fi, &smt->f) && fi.flags.vargs) EMIT_COMMAND("xor rax, rax");
            EMIT_COMMAND("call %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));
            break;
        }
        case LIR_STRT:
        case LIR_FDCL: {
            if (!fi->flags.onlybody) EMIT_COMMAND("%s:", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));
            if (!fi->flags.naked) {
                EMIT_COMMAND("push rbp");
                EMIT_COMMAND("mov rbp, rsp");
                if (
                    b->sarg && 
                    b->sarg->storage.cnst.value > 0
                ) EMIT_COMMAND("sub rsp, %ld", ALIGN(b->sarg->storage.cnst.value, 16));
            }

            break;
        }
        case LIR_STEND: if (fi->flags.naked == 1) break;
                        __attribute__ ((fallthrough));
        case LIR_EXITOP: {
            EMIT_COMMAND("mov rax, 60");
            EMIT_COMMAND("syscall");
            break;
        }
        case LIR_FEND:  if (fi->flags.naked == 1) break;
                        __attribute__ ((fallthrough));
        case LIR_FRET: {
            if (!fi->flags.naked) {
                EMIT_COMMAND("mov rsp, rbp");
                EMIT_COMMAND("pop rbp");
            }
            
            EMIT_COMMAND("ret");
            break;
        }
        case LIR_CQO:  EMIT_COMMAND("cqo");     break;
        case LIR_CDQ:  EMIT_COMMAND("cdq");     break;
        case LIR_SYSC: EMIT_COMMAND("syscall"); break;
        case LIR_OEXT: {
            variable_info_t vi;
            if (
                VRTB_get_info_id(b->farg->storage.cnst.value, &vi, &smt->v) && 
                vi.vmi.used
            ) EMIT_COMMAND("extern %s", vi.name->body); 
            break;
        }
        case LIR_BREAKPOINT: if (CONF_is_debug_compilation()) EMIT_COMMAND("int3 ; %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                  break;
        case LIR_BB:         EMIT_COMMAND("\n; BB%ld:", b->farg->storage.cnst.value);                                                                                                break;
        case LIR_TST:        EMIT_COMMAND("test %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));      break;
        case LIR_XCHG:       EMIT_COMMAND("xchg %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));      break;
        case LIR_MKLB:       EMIT_COMMAND("%s:", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                         break;
        case LIR_SETL:       EMIT_COMMAND("setl %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_SETG:       EMIT_COMMAND("setg %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_STLE:       EMIT_COMMAND("setle %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                    break;
        case LIR_STGE:       EMIT_COMMAND("setge %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                    break;
        case LIR_SETE:       EMIT_COMMAND("sete %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_STNE:       EMIT_COMMAND("setne %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                    break;
        case LIR_SETB:       EMIT_COMMAND("setb %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_SETA:       EMIT_COMMAND("seta %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_STBE:       EMIT_COMMAND("setbe %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                    break;
        case LIR_STAE:       EMIT_COMMAND("setae %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                    break;
        case LIR_NEG:        EMIT_COMMAND("not %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_INC:        EMIT_COMMAND("inc %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_DEC:        EMIT_COMMAND("dec %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JMP:        EMIT_COMMAND("jmp %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JE:         EMIT_COMMAND("je %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JLE:        EMIT_COMMAND("jle %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JNE:        EMIT_COMMAND("jne %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JZ:         EMIT_COMMAND("jz %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JNZ:        EMIT_COMMAND("jnz %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JL:         EMIT_COMMAND("jl %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JG:         EMIT_COMMAND("jg %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JGE:        EMIT_COMMAND("jge %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JA:         EMIT_COMMAND("ja %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JAE:        EMIT_COMMAND("jae %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_JB:         EMIT_COMMAND("jb %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                       break;
        case LIR_JBE:        EMIT_COMMAND("jbe %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_aMOV:
        case LIR_iMOV:       EMIT_COMMAND("mov %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));       break;
        case LIR_MOVZX:      EMIT_COMMAND("movzx %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_MOVSX:      EMIT_COMMAND("movsx %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_MOVSXD:     EMIT_COMMAND("movsxd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));    break;
        case LIR_CVTTSS2SI:  EMIT_COMMAND("cvttss2si %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG)); break;
        case LIR_CVTTSD2SI:  EMIT_COMMAND("cvttsd2si %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG)); break;
        case LIR_CVTSI2SS:   EMIT_COMMAND("cvtsi2ss %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));  break;
        case LIR_CVTSI2SD:   EMIT_COMMAND("cvtsi2sd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));  break;
        case LIR_CVTSS2SD:   EMIT_COMMAND("cvtss2sd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));  break;
        case LIR_CVTSD2SS:   EMIT_COMMAND("cvtsd2ss %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));  break;
        case LIR_fMOV:
        case LIR_fMVf:       EMIT_COMMAND("movsd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_REF:        EMIT_COMMAND("lea %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, LEA_FLAG));      break;
        case LIR_REF_GDREF:  EMIT_COMMAND("lea %s, [%s]", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, LEA_FLAG));    break;
        case LIR_LDREF:      EMIT_COMMAND("mov %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, LDREF_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));    break;
        case LIR_GDREF:      EMIT_COMMAND("mov %s, [%s]", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_PUSH:       EMIT_COMMAND("push %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                     break;
        case LIR_POP:        EMIT_COMMAND("pop %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));                                                                      break;
        case LIR_iADD:       EMIT_COMMAND("add %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_iSUB:       EMIT_COMMAND("sub %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_iMUL:       EMIT_COMMAND("imul %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));      break;
        case LIR_DIV:        EMIT_COMMAND("div %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));                                                                      break;
        case LIR_iMOD:
        case LIR_iDIV:       EMIT_COMMAND("idiv %s", x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));                                                                     break;
        case LIR_CMP:        EMIT_COMMAND("cmp %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));       break;
        case LIR_bAND:
        case LIR_iAND:       EMIT_COMMAND("and %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_bOR:
        case LIR_iOR:        EMIT_COMMAND("or %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));        break;
        case LIR_fADD:       EMIT_COMMAND("addsd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_fSUB:       EMIT_COMMAND("subsd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_fMUL:       EMIT_COMMAND("mulsd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_fDIV:       EMIT_COMMAND("divsd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));     break;
        case LIR_fCMP:       EMIT_COMMAND("ucomisd %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));   break;
        case LIR_bXOR:       EMIT_COMMAND("xor %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_iBLFT:
        case LIR_bSHL:       EMIT_COMMAND("shl %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_iBRHT:
        case LIR_bSHR:       EMIT_COMMAND("shr %s, %s", x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->targ, smt, NO_FLAG));       break;
        case LIR_bSAR:       EMIT_COMMAND("sar %s, %s", x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG), x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));       break;
        case LIR_RAW: {
            string_t* raw_line = create_string(x86_64_gnu_nasm_format_lir_subject(b->farg, smt, NO_FLAG));
            int percent_pos = raw_line->index_of(raw_line, '%');
            if (percent_pos < 0) EMIT_COMMAND("%s", raw_line->body);
            else {
                string_t* replacement = create_string(x86_64_gnu_nasm_format_lir_subject(b->sarg, smt, NO_FLAG));
                string_t* base_line = create_string_from_part(raw_line->body, 0, percent_pos);
                base_line->cat(base_line, replacement);

                const char* suffix = raw_line->body + percent_pos + 1;
                while (str_isdigit((unsigned char)*suffix)) {
                    suffix++;
                }

                string_t* suffix_str = create_string(suffix);
                base_line->cat(base_line, suffix_str);

                EMIT_COMMAND("%s", base_line->body);
                destroy_string(replacement);
                destroy_string(suffix_str);
                destroy_string(base_line);
            }

            destroy_string(raw_line);
            break;
        }
        default: break;
    }

    return 1;
}

/* Emit an independent read-only string into the current assembly section.
Params:
    - `id` - String symbol ID.
    - `smt` - Symtable that stores string information.
    - `output` - Output assembly stream.

Returns 1 if succeeds. */
static int _generate_ro_string(symbol_id_t id, sym_table_t* smt, FILE* output) {
    str_info_t si;
    if (STTB_get_info_id(id, &si, &smt->s) && si.t == STR_INDEPENDENT) {
        EMIT_PART_COMMAND("_str_%li_ db ", si.id);
        for (unsigned long i = 0; i < si.value->size; ++i) {
            fprintf(output, "%u,", (unsigned int)(unsigned char)si.value->body[i]);
        }

        fprintf(output, "0\n");
    }

    return 1;
}

static inline long _array_reserve_size(variable_info_t* vi, array_info_t* ai, token_t* elem_tkn, sym_table_t* smt) {
    long type_size = TPTB_get_memory_size_id(vi->t_id, &smt->t);
    if (type_size != SMT_NULL) return type_size;
    switch (TKN_variable_bitness(elem_tkn, 1)) {
        case TYPE_FULL_SIZE:    return ai->size * 8;
        case TYPE_HALF_SIZE:    return ai->size * 4;
        case TYPE_QUARTER_SIZE: return ai->size * 2;
        default:                return ai->size;
    }
}

/* Emit zero-filled bytes, optionally prefixed with a data label.
Params:
    - `name` - Optional label to emit before the reservation.
    - `count` - Number of zero bytes to emit.
    - `output` - Output assembly stream. */
static inline void _emit_zero_bytes(string_t* name, long count, FILE* output) {
    if (count <= 0) return;
    if (name) EMIT_COMMAND("%s times %ld db 0", name->body, count);
    else      EMIT_COMMAND("times %ld db 0", count);
}

/* Emit an integer value with the NASM directive matching its slot size.
Params:
    - `name` - Optional label to emit before the value.
    - `size` - Slot size in bytes.
    - `value` - Integer initializer value.
    - `output` - Output assembly stream. */
static inline void _emit_typed_value(string_t* name, long size, long long value, FILE* output) {
    const char* op = size == 8 ? "dq" : size == 4 ? "dd" : size == 2 ? "dw" : "db";
    if (name) EMIT_COMMAND("%s %s %lli", name->body, op, value);
    else      EMIT_COMMAND("%s %lli", op, value);
}

/* Emit a typed aggregate initializer using type-layout slots and padding.
Params:
    - `vi` - Variable metadata for the aggregate.
    - `ai` - Array metadata that stores initializer elements.
    - `smt` - Symtable used to resolve type layout.
    - `output` - Output assembly stream.

Returns 1 if succeeds. */
static int _generate_typed_initializer(variable_info_t* vi, array_info_t* ai, sym_table_t* smt, FILE* output) {
    long emitted_end = 0, value_count = list_size(&ai->elems), reserve_size = _array_reserve_size(vi, ai, NULL, smt);
    array_elem_info_t* elem = NULL;
    long value_pos = 0, string_pos = 0;
    symbol_id_t string_owner_id = NO_SYMBOL_ID;

    list_iter_t values;
    list_iter_hinit(&ai->elems, &values);

    EMIT_DATA_LABEL(vi->name->body);
    for (long slot = 0;; slot++) {
        type_init_info_t slot_info = { 0 };
        if (!TPTB_find_type_init_slot(vi->t_id, slot, 0, &slot_info, &smt->t)) break;
        _emit_zero_bytes(NULL, slot_info.slot_off - emitted_end, output);

        type_info_t slot_ti;
        int for_string = (
            TPTB_get_info_id(slot_info.slot_type, &slot_ti, &smt->t)  &&
            slot_ti.t == TYPE_PRIMITIVE                               &&
            slot_ti.body.primitive.token == I8_TYPE_TOKEN             &&
            !slot_ti.ptr
        );

        if (
            !(
                elem && elem->t == ARRAY_ELEM_STRING_TYPE && 
                for_string && string_owner_id == slot_info.slot_owner
            ) && 
            value_pos < value_count &&
            list_iter_next(&values, (void**)&elem)
        ) { /* restore info if this isn't a string */
            string_pos      = 0;
            string_owner_id = NO_SYMBOL_ID;
            value_pos++;
        }

        if (!elem) goto _default_const_type;
        switch (elem->t) {
            case ARRAY_ELEM_STRING_TYPE: {
                if (for_string) {
                    if (!(string_owner_id == NO_SYMBOL_ID || string_owner_id == slot_info.slot_owner)) {
                        _emit_typed_value(NULL, slot_info.slot_size, 0, output);
                        break;
                    }

                    str_info_t si;
                    if (string_owner_id == NO_SYMBOL_ID) string_owner_id = slot_info.slot_owner;
                    _emit_typed_value(
                        NULL, slot_info.slot_size,
                        (STTB_get_info_id(elem->s.s_id, &si, &smt->s) && string_pos < si.value->len(si.value)) 
                            ? si.value->body[string_pos] 
                            : 0,
                        output
                    );
                    string_pos++;
                }
                /* Put a pointer instead of initialization */
                else EMIT_COMMAND("dq _str_%li_", elem->s.s_id);
                break;
            }
            case ARRAY_ELEM_FUNC_TYPE: {
                NASMFMT_emit_typed_func(output, NULL, slot_info.slot_size, elem->s.f_id, smt);
                break;
            }
            default: {
_default_const_type: {}
                _emit_typed_value(NULL, slot_info.slot_size, elem ? elem->s.value : 0, output);
                break;
            }
        }

        emitted_end = slot_info.slot_off + slot_info.slot_size;
    }

    _emit_zero_bytes(NULL, reserve_size - emitted_end, output);
    return 1;
}

/* Emit storage for a non-external variable into the current assembly section.
Params:
    - `id` - Variable symbol ID.
    - `smt` - Symtable used to resolve variable and array metadata.
    - `output` - Output assembly stream.

Returns 1 on success, otherwise 0. */
static int _generate_variable(symbol_id_t id, sym_table_t* smt, FILE* output) {
    variable_info_t vi;
    if (!VRTB_get_info_id(id, &vi, &smt->v) || vi.vfs.ext || !vi.vmi.used) return 0;
    token_t fst_tmptkn = { .t_type = vi.type, .flags = { .ptr = vi.vfs.ptr, .ro = vi.vfs.ro } };

    char name[256] = { 0 };
    snprintf(name, sizeof(name), vi.s_id == 1 ? "%s" : "%s__%li", vi.name->body, vi.v_id);

    if (!TKN_is_one_slot(&fst_tmptkn)) {
        array_info_t ai;
        if (!ARTB_get_info(vi.v_id, &ai, &smt->a)) return 0;
        token_t sec_tmptkn = { .t_type = ai.elements_info.el_type, .flags = { .ptr = ai.elements_info.el_flags.ptr } };
        /* Simple reservation with the unitialized data */
        if (!list_size(&ai.elems)) {
            long reserve_size = _array_reserve_size(&vi, &ai, &sec_tmptkn, smt);
            switch (TKN_variable_bitness(&sec_tmptkn, 1)) {
                case TYPE_FULL_SIZE:    EMIT_COMMAND("%s resq %ld", name, reserve_size / 8); break;
                case TYPE_HALF_SIZE:    EMIT_COMMAND("%s resd %ld", name, reserve_size / 4); break;
                case TYPE_QUARTER_SIZE: EMIT_COMMAND("%s resw %ld", name, reserve_size / 2); break;
                default:                EMIT_COMMAND("%s resb %ld", name, reserve_size);     break;
            }
        }
        /* Reservation with the initialized data */
        else {
            type_info_t ti;
            if (
                TPTB_get_info_id(vi.t_id, &ti, &smt->t) &&
                (ti.t == TYPE_CUSTOM || (
                    ti.t == TYPE_ARRAY &&
                    TPTB_get_type_type_id(TPTB_get_first_child(vi.t_id, &smt->t), &smt->t) == TYPE_CUSTOM
                ))
            ) return _generate_typed_initializer(&vi, &ai, smt, output);

            switch (TKN_variable_bitness(&sec_tmptkn, 1)) {
                case TYPE_FULL_SIZE:    EMIT_PART_COMMAND("%s dq ", name); break;
                case TYPE_HALF_SIZE:    EMIT_PART_COMMAND("%s dd ", name); break;
                case TYPE_QUARTER_SIZE: EMIT_PART_COMMAND("%s dw ", name); break;
                default:                EMIT_PART_COMMAND("%s db ", name); break;
            }

            array_elem_info_t *el = NULL, *last_elem = NULL;
            int el_count = list_size(&ai.elems);
            foreach (el, &ai.elems) {
                last_elem = el;
                switch (el->t) {
                    case ARRAY_ELEM_STRING_TYPE: fprintf(output, "_str_%li_", el->s.s_id); break;
                    case ARRAY_ELEM_FUNC_TYPE: {
                        char buffer[256] = { 0 };
                        fprintf(output, "%s", NASMFMT_format_func_value(el->s.f_id, smt, buffer, sizeof(buffer)));
                        break;
                    }
                    default: fprintf(output, "%lli", el->s.value);                         break;
                }

                if (--el_count) fprintf(output, ",");
            }

            long last_el = ai.size - list_size(&ai.elems);
            if (last_el > 0) fprintf(output, ",");
            while (last_el-- > 0) {
                switch (last_elem ? last_elem->t : ARRAY_ELEM_CONST_TYPE) {
                    case ARRAY_ELEM_STRING_TYPE: fprintf(output, "_str_%li_", last_elem->s.s_id); break;
                    case ARRAY_ELEM_FUNC_TYPE: {
                        char buffer[256] = { 0 };
                        fprintf(output, "%s", last_elem ? NASMFMT_format_func_value(last_elem->s.f_id, smt, buffer, sizeof(buffer)) : "0");
                        break;
                    }
                    default: fprintf(output, "%lli", last_elem ? last_elem->s.value : 0);         break;
                }
                
                if (last_el) fprintf(output, ",");
            }

            fprintf(output, "\n");
        }

        return 1;
    }

    switch (TKN_variable_bitness(&fst_tmptkn, 1)) {
        case TYPE_FULL_SIZE:    EMIT_COMMAND("%s dq %li", name, vi.vdi.defined == DEFINED_VARIABLE ? vi.vdi.definition : 0); break;
        case TYPE_HALF_SIZE:    EMIT_COMMAND("%s dd %li", name, vi.vdi.defined == DEFINED_VARIABLE ? vi.vdi.definition : 0); break;
        case TYPE_QUARTER_SIZE: EMIT_COMMAND("%s dw %li", name, vi.vdi.defined == DEFINED_VARIABLE ? vi.vdi.definition : 0); break;
        default:                EMIT_COMMAND("%s db %li", name, vi.vdi.defined == DEFINED_VARIABLE ? vi.vdi.definition : 0); break;
    }

    return 1;
}

/* Emit assembly for a used CFG function.
Params:
    - `f_id` - Function symbol ID.
    - `cctx` - CFG context with function LIR maps.
    - `smt` - Symtable used to resolve function metadata.
    - `output` - Output assembly stream.

Returns 1 on success, otherwise 0. */
static int _generate_function(symbol_id_t f_id, cfg_ctx_t* cctx, sym_table_t* smt, FILE* output) {
    func_info_t fi;
    if (!FNTB_get_info_id(f_id, &fi, &smt->f) || !fi.flags.used) return 0;
    
    if (!fi.flags.onlybody) {
        if (fi.flags.external == 2) EMIT_COMMAND("extern %s", fi.flags.vname ? fi.virt->body : fi.name->body);
        else { 
            const char* modifier = fi.flags.weak ? ":function weak" : "";
            if (fi.flags.entry)       EMIT_COMMAND("global %s%s", fi.virt->body, modifier);
            else if (fi.flags.global) EMIT_COMMAND("global %s%s", fi.flags.vname ? fi.virt->body : fi.name->body, modifier);
            if (fi.flags.external)    EMIT_COMMAND("extern %s", fi.flags.vname ? fi.virt->body : fi.name->body);
        }
    }

    cfg_func_t* fb;
    if (!map_get(&cctx->fmap, f_id, (void**)&fb) || !fb || !fb->used) return 0;
    iterate_lir_instructions (fb) {
        _convert_lirblock_to_assembly(lh, &fi, smt, output);
    }

    return 1;
}

int x86_64_gnu_nasm_generate_asm(cfg_ctx_t* cctx, sym_table_t* smt, FILE* output) {
    foreach (section_info_t* section, &smt->c.sorted.sectb) {
        if (!section->name->requals(section->name, CONF_get_no_section())) {
            EMIT_COMMAND("section %s", section->name->body);
            if (section->align != SMT_NULL) {
                EMIT_COMMAND("align %i", section->align);
            }
        }
        
        foreach (symbol_id_t id, &section->sorted.vars) {
            _generate_variable(id, smt, output);
        }

        foreach (symbol_id_t id, &section->sorted.strs) {
            _generate_ro_string(id, smt, output);
        }

        foreach (symbol_id_t id, &section->sorted.func) {
            func_info_t fi;
            if (!FNTB_get_info_id(id, &fi, &smt->f)) continue;
            foreach (symbol_id_t l_id, &fi.local) {
                _generate_function(l_id, cctx, smt, output);
            }

            _generate_function(id, cctx, smt, output);
        }
    }

    foreach (lir_block_t* lb, &cctx->outs.lout) {
        _convert_lirblock_to_assembly(lb, NULL, smt, output);
    }

    return 1;
}
