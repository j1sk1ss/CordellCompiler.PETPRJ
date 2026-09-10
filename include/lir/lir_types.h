#ifndef LIR_TYPES_H_
#define LIR_TYPES_H_

#include <config.h>

typedef enum {
    /* Operations */
        /* Commands */
        LIR_FCLL,  // function call
        LIR_ECLL,  // extern function call
        LIR_STRT,  // start macro
        LIR_STEND, // end macro
        LIR_SYSC,  // syscall
        LIR_FRET,  // function ret
        LIR_TST,   // test
        LIR_XCHG,  // xchg
        LIR_CDQ,   // cdq
        LIR_CQO,
        LIR_MKLB,  // mk label
        LIR_FDCL,  // declare function
        LIR_FEND,
        LIR_OEXT,  // extern object
        LIR_FEXT,  // extern function

        LIR_CMP,

        LIR_SETL,
        LIR_SETG,
        LIR_STLE,
        LIR_STGE,
        LIR_SETE,
        LIR_STNE,
        LIR_SETB,
        LIR_SETA,
        LIR_STBE,
        LIR_STAE,

        /* Jump instructions */
        LIR_JMP,  // jmp
        LIR_JZ,
        LIR_JNZ,
        LIR_JL,   // jump if less (signed)
        LIR_JG,   // jump if greater (signed)
        LIR_JLE,  // jump if less or equal (signed)
        LIR_JGE,  // jump if greater or equal (signed)
        LIR_JE,   // jump if equal / zero
        LIR_JNE,  // jump if not equal / not zero
        LIR_JB,   // jump if below (unsigned <)
        LIR_JA,   // jump if above (unsigned >)
        LIR_JBE,  // jump if below or equal (unsigned <=)
        LIR_JAE,  // jump if above or equal (unsigned >=)

        /* Data commands */
        LIR_REF_ARGS,

    /* Register */
        /* Operations */
        LIR_iMOV,     // integer move, x = y
        LIR_aMOV,     // argument load
        LIR_phiMOV,   // [SSA]

        LIR_STARGLD,  // st load
        LIR_STARGRF,  // st ref load

        LIR_VRDECL,   // cnst_x=var_id, y=value
        LIR_VRDEALL,  // cnst_x=var_id, deallocate a variable
        LIR_STRDECL,  // cnst_x=var_id, cnst_y=str_id
        LIR_ARRDECL,  // cnst_x=var_id, var_y=size, list_z=init_elems
        LIR_VLADECL, // cnst_x=var_id, var_y=size

        LIR_STSARG,   // store parameter to a syscall, x, cnst_y=index
        LIR_STFARG,   // store parameter to a function, x, cnst_y=index
        LIR_LOADFARG, // load parameter in a function, v=source_var, cnst_y=index, cnst_z=args-index
        LIR_LOADFRET, // load funcret to a dst, x=target_var

        LIR_TF64,     // x = (f64)y
        LIR_TF32,     // x = (f32)y
        LIR_TI64,     // x = (i64)y
        LIR_TI32,     // x = (i32)y
        LIR_TI16,     // x = (i16)y
        LIR_TI8,      // x = (i8)y
        LIR_TU64,     // x = (u64)y
        LIR_TU32,     // x = (u32)y
        LIR_TU16,     // x = (u16)y
        LIR_TU8,      // x = (u8)y

        LIR_CVTTSS2SI,
        LIR_CVTTSD2SI,
        LIR_CVTSI2SS,
        LIR_CVTSI2SD,
        LIR_CVTSS2SD,
        LIR_CVTSD2SS,
        LIR_MOVSX,
        LIR_MOVZX,
        LIR_MOVSXD,

        LIR_NOT,
        LIR_NEG,
        LIR_INC,
        LIR_DEC,

        LIR_fMOV,      // float move
        LIR_fMVf,      // float to float move
        LIR_REF,       // a = &y
        LIR_REF_GDREF, // a = &*y
        LIR_GDREF,     // get value from address, x = *y
        LIR_LDREF,     // set value by address,   *x = y
        LIR_PUSH,      // push
        LIR_POP,       // pop

    /* Integer */
        /* Binary operations */
        LIR_iADD, // x = y + z  | After selection: x = x + z
        LIR_iSUB, // x = y - z  | After selection: x = x - z
        LIR_iMUL, // x = y * z  | After selection: x = x * z
        LIR_DIV,  // x = y / z  | After selection: x = x / z
        LIR_iDIV, // x = y / z  | After selection: x = x / z
        LIR_iMOD, // x = y % z  | After selection: x = x % z
        LIR_iLRG, // x = y > z  | After selection: x = x > z
        LIR_iLGE, // x = y >= z | After selection: x = x >= z
        LIR_iLWR, // x = y < z  | After selection: x = x < z
        LIR_iLRE, // x = y <= z | After selection: x = x <= z
        LIR_iCMP, // x = y == z | After selection: x = x == z
        LIR_iNMP, // x = y != z | After selection: x = x != z

        /* Logic */
        LIR_iAND, // x = y && z
        LIR_iOR,  // x = y || z

    /* Float */
        /* Binary operations */
        LIR_fADD, // x = y f+ z
        LIR_fSUB, // x = y f- z
        LIR_fMUL, // x = y f* z
        LIR_fDIV, // x = y f/ z
        LIR_fCMP, // x = y f== z

    /* Bits */
        /* Binary operations */
        LIR_iBLFT, // bit left
        LIR_iBRHT, // bit right
        LIR_bAND,  // bit and
        LIR_bOR,   // bit or
        LIR_bXOR,  // bit xor
        LIR_bSHL,  // bitleft
        LIR_bSHR,  // bitright
        LIR_bSAR,  // bitleft unsgn

    /* Other */
    LIR_RAW,
    LIR_BREAKPOINT,
    LIR_SETPOS,

    /* High level operations */
        /* Stack */
        LIR_MKSCOPE,
        LIR_ENDSCOPE,

        /* System */
        LIR_EXITOP, // Exit with farg exit call
        LIR_VRUSE,
        LIR_BB,
} lir_operation_t;

typedef enum {
    LIR_REGISTER,   /* physical register   */
    LIR_STVARIABLE, /* stack variable      */
    LIR_VARIABLE,   /* virtual register    */
    LIR_GLVARIABLE, /* allocated memory    */
    LIR_CONSTVAL,   /* constant value      */
    LIR_NUMBER,     /* constant value      */
    LIR_LABEL,      /* ASM label           */
    LIR_RAWASM,     /* ASM line of code    */
    LIR_MEMORY,     /* stack placement     */
    LIR_FNAME,      /* function name       */
    LIR_STRING,     /* string              */
    LIR_ARGLIST,    /* list of LIR subj    */
    LIR_FPOS,       /* postion in the file */
} lir_subject_type_t;

/* This is the main register's enum.
   It includes CISC and RISC registers (any possible register). 
   
   - Why we use such a big enum here? - The LIR_types file includes
   all essential information for the LIR stage in this compiler, 
   and given the different compiling targets, we need to handle
   all possible registers here. 
   
   - Also, including several acrhs doesn't break a structure of the
   compiler. This file is a main tool-file. */
typedef enum {
    /* CISC related register set */
    RAX,  RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8,  R9,  R10,  R11,  R12,  R13,  R14,  R15,  /* 8 bytes */
    EAX,  EBX, ECX, EDX, ESI, EDI, EBP, ESP, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D, /* 4 bytes */
    AX,   BX,  CX,  DX,  SI,  DI,  BP,  SP,  R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W, /* 2 bytes */
    AL,   BL,  CL,  DL,  SIL, DIL, BPL, SPL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B, /* 1 byte  */
    AH,   BH,  CH,  DH,                                                                    /* 1 byte  */
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,                                        /* float   */

    /* RISC-V related register set */
    RV_X0,  RV_X1,  RV_X2,  RV_X3,  RV_X4,  RV_X5,  RV_X6,  RV_X7,  /* integer */
    RV_X8,  RV_X9,  RV_X10, RV_X11, RV_X12, RV_X13, RV_X14, RV_X15,
    RV_X16, RV_X17, RV_X18, RV_X19, RV_X20, RV_X21, RV_X22, RV_X23,
    RV_X24, RV_X25, RV_X26, RV_X27, RV_X28, RV_X29, RV_X30, RV_X31,
    RV_F0,  RV_F1,  RV_F2,  RV_F3,  RV_F4,  RV_F5,  RV_F6,  RV_F7,  /* float   */
    RV_F8,  RV_F9,  RV_F10, RV_F11, RV_F12, RV_F13, RV_F14, RV_F15,
    RV_F16, RV_F17, RV_F18, RV_F19, RV_F20, RV_F21, RV_F22, RV_F23,
    RV_F24, RV_F25, RV_F26, RV_F27, RV_F28, RV_F29, RV_F30, RV_F31,
    RV_V0,  RV_V1,  RV_V2,  RV_V3,  RV_V4,  RV_V5,  RV_V6,  RV_V7,  /* vector  */
    RV_V8,  RV_V9,  RV_V10, RV_V11, RV_V12, RV_V13, RV_V14, RV_V15,
    RV_V16, RV_V17, RV_V18, RV_V19, RV_V20, RV_V21, RV_V22, RV_V23,
    RV_V24, RV_V25, RV_V26, RV_V27, RV_V28, RV_V29, RV_V30, RV_V31,
} lir_registers_t;

/*
Convert any input register to the register with a fixed size.
Example:
```asm
rax + 4 byte -> eax
ax + 1 byte  -> al
ax + 8 byte  -> rax
```
Params:
    - `reg` - Source register.
    - `size` - Target register size.

Returns register with a fixed size.
*/
lir_registers_t LIR_format_register(lir_registers_t reg, int size);

/*
Check is the LIR operation is a mov-like operation.
Note: Move-like operation is an operation that moves some data
      from a source to a destination.
Params:
    - `op` - LIR operation.

Returns 1 if the operation is a move-like operation.
*/
int LIR_is_movop(lir_operation_t op);

/*
Check is the LIR operation is a write operation.
Note: Write operation is an operation that writes something
      to the first argument in the LIR block.
Params:
    - `op` - LIR operation.

Returns 1 if the operation is a write operation.
*/
int LIR_is_writeop(lir_operation_t op);

/*
Is this a jump-like operation.
Params:
    - `op` - LIR operation.

Returns 1 if the operation is a jump-like operation.
*/
int LIR_is_jumpop(lir_operation_t op);

/*
Check is the LIR operation is a read operation.
Note: Read operation is an operation that uses value from
      the first, the second or the third argument.
Params:
    - `op` - LIR operation.

Returns 1 if the operation is a read operation.
*/
int LIR_is_readop(lir_operation_t op);

/*
Check if the operation will produce some sort of an effect.
For instance: function call can, write and exit as well.
Params:
    - `op` - LIR operation.

Returns 1 if the operation has a side effect.
*/ 
int LIR_has_sideeffect(lir_operation_t op);

#endif