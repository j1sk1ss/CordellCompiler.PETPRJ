#ifndef BUILDER_H_
#define BUILDER_H_

/* Base libs for STDIO with CLI and files        */
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* basics                                        */
#include <std/list.h>

/* Pre-processor part and tokenization part      */
#include <preproc/pp.h>
#include <prep/token.h>
#include <prep/markup.h>

/* Semantic (Static analyzer) setup              */
#include <csa/semantic.h>

/* AST generation part and AST optimization part */
#include <ast/ast.h>
#include <ast/astgen.h>
#include <ast/astgen/astgen.h>

/* HIR generation part and CFG generation part   */
#include <hir/hirgen.h>
#include <hir/hirgens/hirgens.h>
#include <hir/cfg.h>
#include <hir/dump.h>

/* SSA + const fold / prop + TRE + inline + LICM */
#include <hir/ssa.h>
#include <hir/dag.h>
#include <hir/constfold.h>
#include <hir/func.h>
#include <hir/loop.h>
#include <hir/z3opt.h>

/* HLIR generation                               */
#include <lir/lirgen.h>
#include <lir/lirgens/lirgens.h>
#include <lir/dump.h>

/* HLIR copy prop                                */
#include <lir/copyprop.h>

/* HLIR constfold part                           */
#include <lir/constfold.h>

/* From HLIR to LLIR (now we're arch dependent)  */
#include <lir/selector/instsel.h>
#include <lir/selector/memsel.h>
#include <lir/selector/savereg.h>
#include <lir/selector/x86_64_gnu_nasm.h>
#include <lir/selector/i386_gnu_nasm.h>
#include <lir/selector/x86_64_macho_nasm.h>

/* Instruction scheduling                        */
#include <lir/instplan/targinfo.h>
#include <lir/instplan/instplan.h>

/* Liveness analysis + Register allocation       */
#include <lir/dfg.h>
#include <lir/regalloc/ra.h>
#include <lir/regalloc/i386_gnu_precolor.h>
#include <lir/regalloc/x86_64_gnu_precolor.h>
#include <lir/regalloc/regalloc.h>

/* Peephole optimization                         */
#include <lir/peephole/peephole.h>
#include <lir/peephole/x86_64_gnu_nasm.h>

/* Codegen                                       */
#include <asm/asmgen.h>
#include <asm/x86_64_gnu_nasm_asmgen.h>
#include <asm/i386_gnu_nasm_asmgen.h>
#include <asm/x86_64_macho_nasm_asmgen.h>

/* Symtable dump                                */
#include <symtab/dump.h>

#include <gem_data.h>
#define CCPL_VERSION                 "3.7.4:0809.26" // major.minor<.patch> (old version style):ddmm.yy (new version style)
#define CCPL_SPLASH                  "Z3 optimizations" // NULL / Related to the version splash
/* Version logic is next: We have the old style and the new style:
    - Old style is a default version semantics - major-minor-patch style, where major is incremented when
      I've added a lot of new features and they work properly. Also there should be some big shifts in
      logic / syntax / language / optimizations / etc. If there is no huge changes - then this is a minor
      change. If there is just a few bug fixes - it's a patch.
    - New style is a date + mounth + two last digits of a year. There is nothing special - just change it
      when there is any changes in the code just to track the progress and verify whether this is the last
      version of the compiler or not. */

/* Builder remembers where is the default directory for headers (only headers without any executable code)
for any CPL program. It means it is an important data, tho which can be corrupted or rewritten - it will just cause
'header not found' error. */
#ifndef CPL_DEFAULT_INCLUDE_DIR
    #define CPL_DEFAULT_INCLUDE_DIR  "/usr/local/share/cpl/include"
#endif

/* Some headers includes prototypes of functions which implementations are placed in the CPL library. Usually it
is work of the makefile to place libcpl.a by the proper path. But if it can't be performed - change this variable
according you system requirements. */
#ifndef CPL_DEFAULT_RUNTIME_LIB
    #define CPL_DEFAULT_RUNTIME_LIB  "/usr/local/lib/cpl/libcpl.a"
#endif

#ifndef PATH_MAX
    #define PATH_MAX                 4096
#endif

#define OPTION_HELP_SHORT            "-h"
#define OPTION_HELP                  "--help"
#define OPTION_VERSION_SHORT         "-v"
#define OPTION_VERSION               "--version"
#define OPTION_SOMETHING             "--smth"
#define OPTION_SOMETHING_SHORT       "-s"
#define OPTION_PREPROCESS_ONLY       "-E"
#define OPTION_INLUCDE               "-I"
#define OPTION_DEFINE                "-D"
#define OPTION_PRINT_STDLIB          "--print-stdlib-path"
#define OPTION_OUTPUT                "--output"
#define OPTION_ENABLE_AST_ANALYSIS   "--ast-analysis"
#define OPTION_ENABLE_IR_ANALYSIS    "--ir-analysis"
#define OPTION_ANALYSIS_ONLY         "--CSA"
#define OPTION_DEBUG                 "--debug"
#define OPTION_NO_DEBUG              "--no-debug"
#define OPTION_STRICT                "--i-dont-know-what-i-am-doing"
#define OPTION_NON_STRICT            "--i-know-what-i-am-doing"
#define OPTION_NO_OPTIMIZATION       "-O0"
#define OPTION_ROUGHT_OPTIMIZATION   "-O1"
#define OPTION_GOOD_OPTIMIZATION     "-O2"
#define OPTION_MAX_OPTIMIZATION      "-O3"
#define OPTION_ARCH                  "--arch"
#define OPTION_ASM_COMPILER          "--asm-compiler"
#define OPTION_ASM_FORMAT            "--asm-format"
#define OPTION_LINKER                "--linker"
#define OPTION_LINKER_MODE           "--linker-mode"
#define OPTION_LINKER_ARG_SHORT      "-Xlinker"
#define OPTION_LINKER_ARG            "--linker-arg"
#define OPTION_COMPILE_ONLY_SHORT    "-c"
#define OPTION_COMPILE_ONLY          "--compile-only"
#define OPTION_LINKER_NO_PIE         "--linker-no-pie"
#define OPTION_LINKER_PIE            "--linker-pie"
#define OPTION_LINKER_M32            "--linker-m32"
#define OPTION_LINKER_NO_M32         "--linker-no-m32"
#define OPTION_ENTRY_NAME            "--entry-name"
#define OPTION_RO_SECTION            "--ro-section"
#define OPTION_GLOB_SECTION          "--glob-section"
#define OPTION_CODE_SECTION          "--code-section"
#define OPTION_FULL_BYTNESS          "--full-bytness"
#define OPTION_HALF_BYTNESS          "--half-bytness"
#define OPTION_QUART_BYTNESS         "--quart-bytness"
#define OPTION_EIGHT_BYTNESS         "--eight-bytness"
#define OPTION_SYS_TYPE              "--sys-type"
#define OPTION_TRE                   "--tre"
#define OPTION_NO_TRE                "--no-tre"
#define OPTION_FINLINE               "--finline"
#define OPTION_NO_FINLINE            "--no-finline"
#define OPTION_LICM                  "--licm"
#define OPTION_NO_LICM               "--no-licm"
#define OPTION_Z3OPT                 "--z3opt"
#define OPTION_NO_Z3OPT              "--no-z3opt"
#define OPTION_CONSTANT              "--constant"
#define OPTION_NO_CONSTANT           "--no-constant"
#define OPTION_COPYPROP              "--copyprop"
#define OPTION_NO_COPYPROP           "--no-copyprop"
#define OPTION_PEEPHOLE              "--peephole"
#define OPTION_NO_PEEPHOLE           "--no-peephole"
#define OPTION_EMIT_AST              "--emit-ast"
#define OPTION_EMIT_IR               "--emit-ir"
#define OPTION_EMIT_HIR_CFG          "--emit-hir-cfg"
#define OPTION_EMIT_LIR              "--emit-lir"
#define OPTION_EMIT_LIR_CFG          "--emit-lir-cfg"
#define OPTION_EMIT_ASM              "--emit-asm"
#define OPTION_EMIT_SYMTAB           "--emit-symtab"
#define OPTION_AST_OUTPUT            "--ast-output"
#define OPTION_IR_OUTPUT             "--ir-output"
#define OPTION_LIR_OUTPUT            "--lir-output"
#define OPTION_ASM_OUTPUT            "--asm-output"

typedef struct {
    const char* option;
    const char* argument;
    const char* description;
} cli_help_option_t;

typedef enum {
    BUILD_MODE_EXECUTABLE,
    BUILD_MODE_OBJECT,
    BUILD_MODE_ANALYSIS
} build_mode_t;

typedef struct {
    build_mode_t     build_mode;
    struct {
        char*        include;
        char*        stdlib;
        char*        runtime;
        list_t       files;
        list_t       defines;
        list_t       symtab_types;
        char*        output;
        char*        ast_output;
        char*        ir_output;
        char*        hir_cfg_name;
        char*        lir_output;
        char*        lir_cfg_name;
        char*        asm_output;
    } locations;
    struct {
        char*        asm_compiler;
        char*        asm_format;
        char*        linker;
        list_t       linker_args;
        int          linker_use_c_driver;
        int          linker_no_pie;
        int          linker_m32;
    } tools;
    struct {
        char*        entry_name;
        char*        ro_section;
        char*        glob_section;
        char*        code_section;
        long         full_bytness;
        long         half_bytness;
        long         quart_bytness;
        long         eight_bytness;
        arch_type_t  sys_type;
        int          tre                  : 1;
        int          finline              : 1;
        int          z3opt                : 1;
        int          licm                 : 1;
        int          constant             : 1;
        int          peephole             : 1;
        int          copy_prop            : 1;
        int          debug                : 1;
        int          strict               : 1;
        int          emit_ast             : 1;
        int          emit_ir              : 1;
        int          emit_hir_cfg         : 1;
        int          emit_lir             : 1;
        int          emit_lir_cfg         : 1;
        int          emit_asm             : 1;
    } config;
    struct {
        int          ast_analysis         : 1;
        int          hir_analysis         : 1;
        int          show_help            : 1;
        int          show_version         : 1;
        int          print_stdlib         : 1;
        int          show_something       : 1;
        int          preprocess_only      : 1;
    } flags;
} options_t;

#endif
