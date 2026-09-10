# CPL changelog
Logs for the first and second versions are quite short because I do not remember exactly what was introduced and when. However, this page lists most of the major changes. It was created mainly to document the project's evolution clearly, without requiring readers to go through all commits.

----------------------------------------

## Strict vtable index and inheretance
<div class="change-date">Date: 2026-09-11</div>
Vtable methods now have a strict index in a virtual table. Also, container can inheret methods from another container, and given the strict indexing, it allows to use them in shared interfaces:

```cpl
container base {
    @[abstract] @[self] function do(ptr base self) -> i0;
}
container first::base {
    @[override] function do(ptr first self) -> i0 {
    }
}
container second::base {
    @[override] function do(ptr second self) -> i0 {
    }
}

glob first f;
glob second s;

function easy(ptr base b) -> i0 {
    b.do();
}

start() {
    easy(ref f); :/ f.do() /:
    easy(ref s); :/ s.do() /:
}
```

These features aren't well tested and still in progress, which means I'd rather wait till they be complete than use them right now.

## @[override] and @[abstract] annotations
<div class="change-date">Date: 2026-09-10</div>
Now a function can have these annotations, and they work actually the same as they do in another languages. For instance, if we have a prototype in a container:

```cpl
container base {
    @[abstract] @[self]
    function init(ptr base self) -> i0;
}
```

The complier will put `init` to the virtual table (and will enable it for `base` either). But not in `base` instance, it's forbidden to create `base` instance, if it has `abstract` function. </br>
Second annotation `@[override]` works pretty easy. If we have a container which should be used somewhere, where can be used its parent, and we want to use similar methods from a virtual table, we must annotate a function with this annotation:

```cpl
container base {
    @[abstract] @[self]
    function init(ptr base self) -> i0;
}

container instance::base {
    i32 body;
    @[override]
    function init(ptr instance self) -> i0;
}

function instance::init(ptr instance self) -> i0 {
    self.body = 0 as i32;
}

function pipe(ptr base b) -> i0 {
    b.init();
}

start() {
    instance i;
    pipe(ref i);
}
```

In this example, the compiler will put `init` somewhere in the `instance`s virtual table, and will do the same for all childrens of `base`. This allows to invoke `init` safely, because we known where this method is in the provided container. </br>
**P.S.: Inheretence still in progress, I've implemented override and abstract for functions, not for containers.**

## Virtual table in a container
<div class="change-date">Date: 2026-09-10</div>
Containers now have an opportunity to include a virtual table. This is a High IR concept which extends type size to store linked functions. At declaration, the compiler iterates thru linked methods and load them into this table. By default it's a hidden feature and won't change anything, but this is a base for future inheretance logic.

## Pop register and volatile
<div class="change-date">Date: 2026-09-05</div>
I've fixed some bugs with the pop register annotation and added the volatile annotation. At this point, this annotation only marks a variable as used before final optimizations, which preserves it from being deleted by the compiler. 

## Z3 optimizations
<div class="change-date">Date: 2026-08-28</div>
CPL now supports Z3-backed optimization. For now, this only covers dead-branch elimination: if Z3 proves that a branch is dead, the compiler removes it. For instance:

```cpl
i32 a = 10;
if a < 0; { :/ <- Delete this branch and the whole expression /:
    :/ something /:
    return 1;
}

return 2;
```

# Version v3.7
<div class="change-date">Date: 2026-08-23</div>
CPL is now a strongly typed language! Function pointers now use this form:

```cpl
function foo(i32 a, i32 b) -> i32;
start() {
    ptr fn(i32,i32)i32 func_pointer = foo;
}
```

----------------------------------------

## Nested containers
<div class="change-date">Date: 2026-08-21</div>
Containers are useful, but if they can be created only at the top level, their abilities are limited. This update changes that: users can now create nested containers.

```cpl
container outer {
    container nested {
        i32 a;
    }
    i32 a;

    @[self]
    function new(ptr outer self, i32 a) {
        nested n = { a };
        return a + n.a + self.a;
    }
}

start() {
    outer a = { 1 };
    exit a.new(1) as u8; :/ 3 /:
}
```

## NotNull
<div class="change-date">Date: 2026-08-17</div>
CSA allows us to track many things, including nullability in external functions where no function body is available. For instance:

```cpl
function malloc(i32 size) -> ptr i0;
function free(@[not_null] ptr i0 p) -> i0;

start() {
    ptr i0 buffer = malloc(10);
    buffer = 0;
    free(buffer); :/ <- NULL! /:
}
```

## CSA
<div class="change-date">Date: 2026-08-16</div>
Cordell Static Analyzer now has its own compiler parameter: `--CSA`. It can also find edge cases related to function calls. For instance:

```cpl
function foo(ptr i32 a) -> i0 {
    i32 b = dref a;
}

i32 a = 0;
ptr i32 b = ref a;
foo(b); :/ Won't fire a warning that foo will dereference 0 /:

b = 0;
foo(b); :/ Will fire a warning /:
```

## Compact switches
<div class="change-date">Date: 2026-08-12</div>
Switches can now be compact, especially with annotations:

```cpl
function foo(i32 b) -> i0 {
    i32 a;
    @[no_fall] @[straight]
    switch b; {
        case 1; a = 1;
        case 2; a = 2;
        default a = 3;
    }
}
```

## Local and global globals
<div class="change-date">Date: 2026-08-08</div>
Local `glob`s now act like static variables in C. For instance, you can create multiple local globals in different functions:

```cpl
function foo() { glob i32 a = 1; }
function bar() { glob i32 a = 1; }
```

However, there is no way to create multiple global globals with the same name:

```cpl
glob i32 a = 1;
glob i32 a = 1;
```

In the end, local globals behave like other variables with one difference: they live in a section, not on the stack. That means you can use them when you do not want to spend stack space on something large.

## For-like loops via annotations
<div class="change-date">Date: 2026-07-26</div>
Loops now accept a start variable and a step variable in the counter annotation, as an alternative to integer literals:

```cpl
@[counter(10, 1)] loop {}

i32 a; i32 b;
@[counter(a, b)] loop {}
```

## String size cap
<div class="change-date">Date: 2026-07-23</div>
Strings now track the provided size. If a user provides size `0`, the compiler allocates enough space for the entire string. Otherwise, it truncates the string:

```cpl
arr msg[0, i8] = "Hello world! This is a big string!";   :/ Hello world! This is a big string! /:
arr msg[10, i8] = "Hello world! This is a big string!";  :/ Hello wor                          /:
```

## Strings initialization
<div class="change-date">Date: 2026-07-22</div>
The compiler now allows containers to be initialized with strings:

```cpl
container asd {
    ptr i8 a;
    arr    b[10, i8];
}

glob asd a = { "asd", "asd" };
```

This does not require the `ref` keyword because of IR-specific constraints. The IR requires a single instruction to initialize a global variable. Since strings are read-only, I decided to allow this logic in the compiler.

## De-ast-detectorization
<div class="change-date">Date: 2026-07-19</div>
Several AST-level detectors were removed. They did not work properly because they did not have enough information about the code.

## Overloads, generics, and standard library
<div class="change-date">Date: 2026-07-14</div>
At this point, there is no valid way to properly handle a case like this:

```cpl
function foo<T>(T a, i32 b) -> T {
}
function foo<U>(U a, i64 b) -> U {
}
foo<i32>(1, 1 as i32);
foo<i32>(1, 1 as i64);
```

That is why I decided to forbid this usage pattern. </br>
I also decided to start working on my standard library, which means there is a lot of work to do.

## CPL standard library!
<div class="change-date">Date: 2026-07-02</div>
The compiler now has a library that provides a major part of the C/glibc interface. To use it, take the glibc-style header name and append `_h`:

```cpl
#include <stdio_h.cpl>
start() {
    printf(ref "Hello world!\n");
}
```

The preprocessor now also accepts `<>` headers, and the Makefile installs the library under the `share` location.

## No more AST-level optimization
<div class="change-date">Date: 2026-06-15</div>
The oldest optimizations now deleted from the compiler. They weren't used in the pipeline, which means this change doesn't affect anything.

## Arch flags in the preprocessor
<div class="change-date">Date: 2026-06-15</div>
The preprocessor now has a set of pre-initialized flags and defines. One group is the set of architecture flags (`CCPL_MACHO64`, `CCPL_GNU64`, `CCPL_GNUI386`, and `CCPL_WINDOWS64`):

```cpl
#ifdef CCPL_MACHO64
    :/ MACHO64 specific code /:
#endif
#ifdef CCPL_GNU64
    :/ GNU x86_64 specific code /:
#endif
#ifdef CCPL_GNUI386
    :/ GNU i386 specific code /:
#endif
#ifdef CCPL_WINDOWS64
    :/ Windows specific code /:
#endif
```

P.S.: *At this point, GNU x86_64/i386 are the same (in general), but there is no way to check two flags at the same time.*

## Section alignment
<div class="change-date">Date: 2026-06-12</div>
Section annotation now can have an align modifier which helps us to align an entire section. For instance:

```cpl
@[section(".text", 16)]
glob i32 bloated = 0;

@[section(".text", 16)]
function aligned_func() {
    i32 a;
    arr test[10, i32];
}
```

The second optional parameter in the section annotation is now responsible for section alignment. </br>
Why do we need this? The `GRUB2` bootloader requires us to write a boot script that looks like this:

```asm
; code ...
section .bss
align 16
stack_bottom:
	resb 16384 ; 16 KiB
stack_top:
; code ...
```

Previously, there was no way to set section alignment. Now this is possible.

## only_body annotation
<div class="change-date">Date: 2026-06-11</div>
Sometimes it is useful to emit low-level information that is neither a function nor a variable. For instance: `[bits 16]`. CPL functions can now be used with the `only_body` annotation:

```cpl
@[only_body] :/ This function should be glob to be preserved in the final code /:
glob function __head() {
    asm() { "[bits 16]" }
}
start() {
    exit 1;
}
```

The code above works as a file header:

```asm
[bits 16]
global _start
_start:
    ; logic
```

The annotation tells the compiler to ignore all information about the `__head` function and use only its body. The important part is that `__head` still exists, which means the compiler can still optimize it.

## :: for implementation
<div class="change-date">Date: 2026-06-10</div>
Instead of the `impl` annotation, I decided to use `::`:

```cpl
container math {
    function add(i32 a, i32 b);
}

function math::add(i32 a, i32 b) {
    return a + b;
}

start() {
    math mt;
    mt.add(1, 1);    : Instance :
    math::add(1, 1); : Static   :
}
```

P.S.: *Comments are the same; strict `::` is now considered container access.*

## Impl annotation
<div class="change-date">Date: 2026-06-08</div>
A container can include a function implementation or only a prototype:

```cpl
container math {
    function add(i32 a, i32 b) {
        return a + b;
    }
}
```

An inline implementation is emitted into every object file. This is convenient when you do not want to share symbols across files, but now you can also create a separate implementation:

```cpl
container math {
    glob function add(i32 a, i32 b);
}

@[impl(math)]
function add(i32 a, i32 b) {
    return a + b;
}
```

## Union annotation
<div class="change-date">Date: 2026-06-05</div>
A container can be a union:

```cpl
@[union]
container cntr {
    i32    a;
    i8     b;
    ptr i0 addr;
} :/ Size is 8 /:
```

Unions also accept `@[align(X)]` and `@[like_c]` annotations, which makes them useful in systems development:

```cpl
@[like_c]
@[union]
container dev {
    i8  size8;
    i16 size16;
    i32 size32;
    i64 size64;
}
```

## ABI and weak annotations
<div class="change-date">Date: 2026-06-04</div>
Two new function annotations were added for low-level interop and linker-visible metadata:

```cpl
@[abi]
extern function something(...) -> i0;

@[weak]
function fallback() -> i0 {
}
```

The `abi` annotation marks a function as ABI-compatible, while `weak` lets the assembly backend emit weak symbol information for the linker.

## Containers!: Arrays
<div class="change-date">Date: 2026-05-31</div>
Containers can now be stored in arrays:

```cpl
container str {
    ptr i8 body;
    i32    size;
}

arr smth[10, str];
```

This update also changed the type system, so now we can create matrices:
```cpl
arr smt[10, arr[10, i32]];
smt[0][0] = 0;
```

This is a major change, but it does not change anything visible, so it may not receive much attention even though it is important.

## Containers!: Like C annotation
<div class="change-date">Date: 2026-05-28</div>
Supporting the C library requires containers and the compiler ABI to work with C properly. Now containers can be marked as C-like:

```cpl
@[like_c]
container a {
}
```

## Containers!: Methods and Static functions
<div class="change-date">Date: 2026-05-23</div>
A container can contain a function. There is no hidden pointer or similar object-model machinery:

```cpl
container node {
    function init(ptr node self) -> i0 {
    }
}

start() {
    node a;
    a.init();
}
```

The parser simply passes a self node to the called object, with or without a reference. The first `self` argument is mandatory if you want to work with a container instance. If you only want a convenient namespace, use the `static` annotation:

```cpl
container std {
    @[static]
    function sum(i32 a, i32 b) -> i32 {
        return a + b;
    }
}

start() {
    std s;
    s.sum();
}
```

I am not planning to create static containers for now, which means there is no way to create a real namespace. Containers are C-like structures with some additional features. </br>
Generics also work the same way with containers:

```cpl
container math {
    @[static]
    function sum<T>(T a, T b) -> T {
        return a + b;
    }
}

start() {
    math m;
    i32 a = m.sum<i32>(1, 1);
}
```

## Containers!: Basics
<div class="change-date">Date: 2026-05-23</div>
It is convenient to have a structure that can store different types. CPL now supports this syntax:

```cpl
container node {
    i32 a;
    i32 b;
}

node nd;
nd.a = 0;
nd.b = 0;
```

At this point, this is only a container, which means it can store only primitives and pointers. For instance:
```cpl
container a {
}
container s {
    ptr i32 a;
    f64 b;
    ptr i8 msg;
    ptr a d;

    str msg;       :/ Illegal! /:
    a nested;      :/ Illegal! /:
    arr k[10, u8]; :/ Illegal! /:
}
```

You also cannot create an array of containers yet:
```cpl
container a {
}

arr b[10, a]; :/ Illegal! /:
```

*P.S.:* This is the first version of containers.

## inline annotation
<div class="change-date">Date: 2026-05-21</div>
Added an annotation that helps the compiler decide whether a function should be inlined. This annotation works similarly to Rust's inline annotation. For instance:

```cpl
@[inline] function foo(); :/ Function will be inlined with higher odds /:
@[inline(always)] function bar(); :/ Will inline the function under any circumstances /:
@[inline(never)] function baz(); :/ Will skip this function /:
@[inline(model)] function chloe(); :/ ! Experimental ! The model will decide whether a function will be inlined /:
```

This is an experiment. I would like to see how compact trained models can perform inside optimizing compilers. I would also like to understand how portable such a model is. At this point, I am working mainly with C-based data and trying to implement the obtained models in a C-like compiler with acceptable results. In the future, I will try the same experiment with Python and Lua.

## actual poparg
<div class="change-date">Date: 2026-05-20</div>
The `poparg` annotation now works, but differently from ABI `va_list`. The original idea was to create a convenient tool for using variadic arguments, and at this moment that cannot be done cleanly through ABI support alone. </br>
The ABI requires us to support register arguments as well as stack arguments. That requires a structure such as `va_list`, which is not convenient here. For now, I force the compiler to push all parameters to the stack when calling a variadic function.

## neg
<div class="change-date">Date: 2026-05-19</div>
The compiler now has a mechanism to invert a variable at the bit level. To do this, use the following keyword:

```cpl
u8 a = neg 1; : 254 :
```

P.S.: The `not` command looks similar to `neg`, but works much differently. `not` is boolean-like negation, while `neg` flips bits. On x86 targets CPL `neg` is emitted with the machine `not` instruction, not the arithmetic `neg` instruction.

## i386
<div class="change-date">Date: 2026-05-16</div>
The compiler now has i386 as a target architecture.

# Version v3.5
<div class="change-date">Date: 2026-05-06</div>
The compiler now supports polymorphic parameters. This makes it possible to implement generic types and functions, opening the way to structures and user-defined types. The preparation phase, which runs before the AST phase, was refactored. </br>
The biggest change is new syntax like this:

```cpl
function foo<T>(T a) -> T {
    return a;
}
foo<i32>(1);
foo<i8>(1);
```

It creates a function for each unique set of types. At this point, it can work with any type that can be used as an argument.

----------------------------------------

## Z3
<div class="change-date">Date: 2026-05-02</div>
The Python Z3 wrapper is now available for use during deep static analysis.

## Inefficient switch
<div class="change-date">Date: 2026-04-27</div>
Check whether it is better to add an annotation.

## syscall checker
<div class="change-date">Date: 2026-04-18</div>
The static analysis tool now accepts the `syscall` keyword. At this point, we have Mach-O support because I am testing this on my MacBook. </br>
This support means that the analyzer checks whether all arguments have the correct types for the selected syscall number. For instance:

```cpl
syscall(0x2000004, 1, 1, 12);
```

This is a Mach-O `write` syscall. The second argument is STDIN/STDOUT destination, the third is a buffer pointer, and the last is the buffer size. In this case, I passed `1` as a buffer, which reveals that I need to cast it at least:

```cpl
start() {
    syscall(0x2000004, 1, 1, 12);
    syscall(0x2000004, 1, 1 as ptr i0, 12 as u64);
}

:/ OUTPUT
[WARNING] [2:41] Syscall with number 4 has some wrong typed arguments! It can lead to UB, consider to cast them:
          [2:41]     Argument 2 should have the 'u64' type, but the 'i8' is provided! Consider to cast it.
          [2:41]     Argument 1 should have the 'ptr i0' type, but the 'i8' is provided! Consider to cast it.
/:
```

Information about these syscalls is currently stored directly in the compiler. I would like to move it to a file later. </br>
This small change addresses cast-related problems. The compiler depends heavily on correct typing because it chooses registers based on variable types; even a wrong cast can pass garbage from the stack to a syscall.

## No more 'ptr str'
<div class="change-date">Date: 2026-04-18</div>
I ran into many problems while supporting `ptr str`. It does the same thing as `ptr i8`, but it has to be treated separately, which creates many complex cases. Hence, it is much easier to remove the `ptr str` syntax and suggest that users use `ptr i8` instead. This does not change code generation or logic; it only means that `str` objects can live only on the stack, just like `arr` objects. </br>
You might suggest removing `str` completely, but I have future plans to support built-in string comparison. For instance, my plan is to recognize the code below as valid:

```cpl
str msg = "Hello world!";
if msg == some_pointer as ptr i8; {
    :/ Do something /:
}
```

An operation like the one above **must** include an object that tells the compiler we are working with a string object. This string object will contain all essential information for code and operation generation, such as the string length, string body, and so on.

## Brainfuck!
<div class="change-date">Date: 2026-04-17</div>
The compiler now compiles a Brainfuck interpreter! The code below works.

```cpl
function strlen(ptr i8 s) -> i32 {
    i32 l = 0;
    while dref s; {
        l += 1 as i32;
        s += 1 as ptr i8;
    }

    return l;
}

function putc(i8 c) -> i0 {
    syscall(0x2000004, 1, ref c, 1);
}

glob arr tape[30000, i8];
glob arr bracketmap[10000, i32];
glob arr stack[10000, i32];

start(i32 argc, ptr ptr i8 argv) {
    i32 pos = 0;
    i32 stackptr = 0;
    i32 codelength = strlen(argv[1]);
    while pos < codelength; {
        @[no_fall]
        @[straight]
        switch argv[1][pos]; {
            case '['; {
                stack[stackptr] = pos;
                stackptr += 1;
            }
            case ']'; {
                if stackptr > 0; {
                    stackptr -= 1;
                    i32 matchpos = stack[stackptr];
                    bracketmap[pos] = matchpos;
                    bracketmap[matchpos] = pos;
                }
            }
        }
        
        pos += 1;
    }
    
    i32 pc = 0;
    i32 pointer = 0;
    while pc < codelength; {
        @[no_fall]
        switch argv[1][pc]; {
            case '>'; {
                pointer += 1;
                pc += 1;
            }
            case '<'; {
                pointer -= 1;
                pc += 1;
            }
            case '+'; {
                tape[pointer] += 1;
                pc += 1;
            }
            case '-'; {
                tape[pointer] -= 1;
                pc += 1;
            }
            case '.'; {
                putc(tape[pointer]);
                pc += 1;
            }
            case '['; {
                if not tape[pointer]; pc = bracketmap[pc];
                else pc += 1;
            }
            case ']'; {
                if tape[pointer]; pc = bracketmap[pc];
                else pc += 1;
            }
            default {
                pc += 1;
            }
        }
    }

    exit 0;
}
```

## Refactoring and bugfixes
<div class="change-date">Date: 2026-04-17</div>
Fixed many bugs in register allocation, liveness analysis, and related areas. Nothing special was implemented.

## Number types
<div class="change-date">Date: 2026-03-29</div>
Numbers in the compiler were always treated as `i64` values. This is not incorrect, but it is not precise either. To address this, numbers now have types based on their values. For instance:

```cpl
100 : i8 :
130 : u8 :
1000 : i16 :
50000 : u16 :
: etc :
```

P.S.: This allows the compiler to move closer to strong typing.

## Large comment blocks
<div class="change-date">Date: 2026-03-29</div>
The compiler now supports a second form of comment creation:

```cpl
: COMMENT LINE :

:/
This is: a comment too!
/:
```

## Sizeof as a keyword
<div class="change-date">Date: 2026-03-27</div>
Moved from the `sizeof` annotation to the `sizeof` keyword.

```cpl
: old i32 len = @[sizeof]"Hello world"; :
i32 len = sizeof("Hello world");
```

## Hidden return
<div class="change-date">Date: 2026-03-25</div>
Similarly to Rust, the compiler now recognizes this syntax:

```cpl
function simple() {
    10 + 10;
}
```

The `return` statement is now optional when the last expression in a function block is the returned value. This feature is especially useful for lambda functions:

```cpl
: function logic(i32 a, i32 b, ptr i0 f); :
logic(10, 123, (i32 a, i32 b) => { a + b * 100; });
: function foo(ptr i0 f); :
foo(() => 10);
foo((i32 a) => a * a);
```

## Lambdas!
<div class="change-date">Date: 2026-03-24</div>
The compiler now supports lambda functions. This is syntactic sugar because lambdas copy the behavior of local functions. The syntax is:

```cpl
ptr i0 f = (i32 a) => { return a; }
ptr i0 f = () => { return 10; }
```

To start a lambda function definition, create a variable placeholder. For example, create an empty holder (`i8 a`). Then use the `=>` symbol to define logic in a scope. </br>
Here is a set of possible lambdas:

```cpl
function foo(ptr i0 f);
foo((i32 a, i32 b) => { if 1; return a; return b; });
foo(
    (i32 a, i32 b) => {
        if 1; {
            return a * 10;
        }
        return b - 10 * a;
    }
);
```

## Constant propagation through parameter lists
<div class="change-date">Date: 2026-03-20</div>
The constant propagation module now supports propagation through function call arguments. If function calls have the same argument in the same position, the pass propagates that input argument into the function and enables further folding. For instance:

```cpl
function foo(i32 a) {
    return a + 10;
}

start() {
    foo(10);
}
```

In the code above, `10` is propagated into the function, which produces code like this:

```cpl
function foo(i32 a) {
    return 20;
}

start() {
    foo(10);
}
```

This is a simple example, but it also works with more complex cases.

## HIR static analyzer and HIR locations
<div class="change-date">Date: 2026-03-15</div>
File-location information is now carried into HIR through a special operation and subject. This feature allows static analysis to expand into the HIR stage. For testing, I added a null-dereference tester and an `if` tester. </br>
The code now looks like this:

```
setpos, line=1, column=7, file=<unknown>
{
setpos, line=1, column=7, file=<unknown>
setpos, line=1, column=7, file=<unknown>
    start {
        {
setpos, line=2, column=9, file=<unknown>
            {
setpos, line=2, column=9, file=<unknown>
setpos, line=2, column=9, file=<unknown>
                i32s %0 = alloc;
setpos, line=2, column=9, file=<unknown>
setpos, line=2, column=22, file=<unknown>
setpos, line=2, column=18, file=<unknown>
                i32t %1 = i8n 1 as i32;
setpos, line=2, column=12, file=<unknown>
                i32s %0 = i32t %1;
setpos, line=3, column=10, file=<unknown>
setpos, line=3, column=13, file=<unknown>
                exit i32s %0;
            }
        }
    }
}
```

## not_lazy
<div class="change-date">Date: 2026-03-12</div>
Lazy logical operators are the default in C-like languages, and CPL now supports an alternative approach. With this annotation, the compiler generates both sides before the final evaluation. </br>
To see the difference, consider this example:

```cpl
function foo(); : Always returns 0 :
function foo(i32 a);
i32 a = foo() && foo(10);
i32 b @[not_lazy] (foo() && foo(10));
```

The `a` variable obtains `0` before `foo(10)` is evaluated. In contrast, the `b` variable obtains the same value, but with `foo(10)` evaluated.

## There is no basic scope anymore!
<div class="change-date">Date: 2026-03-12</div>
The basic scope is finally gone. The syntax is now much closer to C:

```cpl
function foo();
start() {
    foo();
    exit 1;
}

: Instead of :
{
    function foo();
    start() {
        foo();
        exit 1;
    }
}
```

P.S.: *This does not affect existing code or its behavior; it just looks better.*

## Remove section, align and import keywords
<div class="change-date">Date: 2026-03-11</div>
Section and align were fully duplicated by annotations, which are more convenient. The `import` keyword no longer fits the language design at this point; headers fit better. </br>

```cpl
: OLD
align(16) {
    i32 a;
    i32 b;
    section(".bss") {
        i32 c;
        i32 d;
    }
}
:

@[align(16)] i32 a;
@[align(16)] i32 b;
@[align(16)] @[section(".bss")] i32 c;
@[align(16)] @[section(".bss")] i32 d;
```

## sizeof
<div class="change-date">Date: 2026-03-09</div>
With the `sizeof` annotation, it is now possible to support this code:

```cpl
arr data[256, i32];
i32 index = 0;
while index < @[sizeof]data; {
    data[index] = 0;
}
```

This annotation will give you exact size in bytes of an object:

```cpl
@[sizeof]1; : Will return max bytness of the system :

ptr i32 a;
@[sizeof]a; : Will return max bytness of the system :

arr b[10, ptr i32];
@[sizeof]b; : Will return max bytness of the system * 10 :

str msg = "Hello world!";
@[sizeof]msg; : Will return 13 :

function foo();
@[sizeof]foo; : Will return max bytness of the system :
```

## Register
<div class="change-date">Date: 2026-03-04</div>
The compiler now has the `register` annotation. It links a variable, and only a variable, to a specific system register index. </br>
P.S.: *Strongly depends on the target architecture.*

```cpl
#define RAX 0
@[register(RAX)] i32 a = 1;
:
mov rax, 1
:
```

## Cold/Hot
<div class="change-date">Date: 2026-03-03</div>
With the `hot` and `cold` annotations, it is now possible to generate cold sections. This works with simple `if`/`else` statements and switches. For instance:

```cpl
@[cold] if 1; { : IF1 :
    : something :
}
else { : ELSE1 :
    : something hot :
}
```

The `cold` annotation moves the `IF1` branch to the end of a function and keeps the `ELSE1` branch in place. The same applies to `switch`, with one difference: `switch` does not support `hot` annotations because it is not clear which sections should be moved to cold storage. For instance:

```cpl
@[no_fall]
switch 1; {
    @[cold] case 1; {}
    case 2; {}
    default {}
}
```

**Note:** This code moves the `case 1;` branch to the end of a function. Consider `no_fall` an essential annotation in such cases.

# Version v3.4
<div class="change-date">Date: 2026-02-28</div>
CPL is a systems programming language, which means it should be able to handle tasks such as bootloader creation, VGA printing, filesystems, and similar low-level work. To support these things, the compiler and language now support the following features:

## Align and Section keyword
<div class="change-date">Date: 2026-02-28</div>
For systems programming, it is essential to have `section` and `align` modifiers. The compiler now supports this syntax:

```cpl
section(".text") {
    glob i32 a;
    align(16) glob i32 b;
    align(64) {
        glob i32 c;
        glob i64 d;
    }
}
```

**Note:** `align` and `section` scopes do not affect the target variable declaration scope. This means they do not increase a variable's scope ID.

## Annotations
<div class="change-date">Date: 2026-02-28</div>
The second way to define `section`, `align`, and related metadata is through annotations. The syntax is similar to Rust:

```cpl
@[section(".text")]
@[naked]
function foo();
@[align(16)] glob i32 a;
```

At this moment, the compiler supports these annotations:
- `naked` - disables all entry and exit routines in the final assembly code for the annotated function.
- `align` - does the same work as the `align` keyword.
- `section` - does the same work as the `section` keyword.
- `address` - places a function at a specific address.
- `entry` - marks a function as the code entry point.

The `align` and `section` keywords do the same work as the annotations, but in a more convenient way. An annotation cannot be applied to many declarations or several functions at once.

----------------------------------------

## i0 variable type
<div class="change-date">Date: 2026-02-25</div>
The `i0` variable type can now be used for variables.

```cpl
ptr i0 a;
```

It must be used with the `ptr` keyword. Otherwise, it will not work. </br>
Also, function pointers are now `ptr i0` by default.

```cpl
function foo();
ptr i0 a = foo;
a();
```

## Local functions
<div class="change-date">Date: 2026-02-25</div>
As in Rust, functions can define other functions in their body:

```cpl
function foo() -> i0 {
    function bar() -> i32 {
        return 32;
    }
    return bar();
}
```

These functions can be optimized like regular functions. At this moment, they do not have access to outer variables:

```cpl
function var_decl() -> i0 {
    i32 a;
    function var_try_to_use() -> i0 {
        a += 1; : <= Illegal :
    }
    var_try_to_use();
}
```

These functions can also be used as return values when you want to implement something like a function factory:

```cpl
function factory(i32 key) -> ptr u64 {
    switch key; {
        case 1; {
            function foo() {
                return 1;
            }
            return foo;
        }
        default; {}
        case 2; {
            function bar() {
                return 2;
            }
            return bar;
        }
    }
}

start() {
    exit factory(1)();
}
```

## Scope functions
<div class="change-date">Date: 2026-02-25</div>
At this moment, this is a mostly useless compiler feature:

```cpl
{
    function foo();
}
{
    function foo();
}
```

Scopes now participate in function symbol resolution.

## Function return type new semantic
<div class="change-date">Date: 2026-02-17</div>
CPL semantics have moved a bit toward Rust. Now, instead of `=>` for a return type, use `->`.

```cpl
function foo() -> i0; : instead of function foo() => i0; :
```

This was not possible before because of tokenizer limitations. Now it is possible.

## Pointer calls
<div class="change-date">Date: 2026-02-17</div>
The compiler now supports function pointers! One disadvantage is that pointed-to functions cannot support default arguments or function overloads. Consider this example:

```cpl
{
    function foo(i32 a) => i32 {
        return a;
    }

    start() {
        ptr u64 a = foo;
        i32 b = a(10);
    }
}
```

Here we put the `foo` function pointer into variable `a`. Then we can invoke the function through that pointer. Obviously, we cannot expect default arguments here, at least at this moment. Also, function pointers are `u64` pointers, so store them in `ptr u64` variables because the compiler does not support signature types yet. </br>
Old, traditional function calls work the same as before this update.

## Matrices!
<div class="change-date">Date: 2026-02-15</div>
The compiler now supports multi-indexing!

```cpl
extern ptr ptr u32 matrix;
matrix[0][0] = 1;
```

To declare matrices, you still need to use regular arrays:

```cpl
arr a[2, u32];
arr b[2, u32];
arr c[2, ptr u32] = { ref a, ref b };

c[0][0] = 1; : a[0] :
c[1][0] = 1; : b[0] :
```

## if-elseif-else syntax support
<div class="change-date">Date: 2026-02-13</div>
The compiler now supports this code snippet:

```cpl
if a; {
}
else if b; {
}
else if c; {
}
:/ ... /:
else {
}
```

## Function overloads (basics)
<div class="change-date">Date: 2026-02-17</div>
CordellCompiler now supports overloaded functions. The syntax is below:

```cpl
{
    function chloe(i32 a, i32 b = 10) => i0;
    function chloe(i64 a, i32 b = 10) => i0;
    start() {
        chloe(10 as i32);
        chloe(10 as i64);
        exit 0;
    }
}
```

The idea is that the HIR level can choose a function that supports the input arguments. To help the compiler, use the `as` keyword. Additionally, functions with a default argument, for instance:

```cpl
function chloe(i32 a, i32 b = 10) => i0;
```

cannot support such polymorphism. The example above works because the argument count is the same. If we consider the next example:

```
function chloe(i32 a, i16 b = 10) => i0;
function chloe(i32 a, i32 b = 10, i32 c = 10) => i0;
```

this mechanism won't work.

## Variadic arguments
<div class="change-date">Date: 2026-01-19</div>
CPL now supports variadic arguments! The syntax is similar to C:

```cpl
function foo(...) -> i0 {
}
```

To pop an argument from this set, use the `poparg` keyword:

```cpl
function max(...) -> i0 {
    i32 chloe = poparg as i32;
}
```

The `poparg` keyword can also be used in any function with arguments. It simply replaces normal argument loading:

```cpl
function foo(i32 a, i32 b) {
    i32 c = poparg as i32; : a :
    i32 d = poparg as i32; : b :
}
```

# Version v3.3
<div class="change-date">Date: 2026-01-08</div>
CPL preprocessing directives and a new include system. </br>
The compiler now supports directives such as `define`, `undef`, `ifdef`, and `ifndef`. It also supports the `include` statement. For more information, check the main README file. </br>
For instance, consider this piece of code:

```cpl
: string_h.cpl :
{
#ifndef STRING_H_
#define STRING_H_ 0
    : Get the size of the provided string
      Params
        - `s` - Input string.

      Returns the size (i64). :
    function strlen(ptr i8 s) => i64;
#endif
}

: print_h.cpl :
{
#ifndef PRINT_H_
#define PRINT_H_ 0
    #include "string_h.cpl"

    : Basic print function that is based on
      a syscall invoke.
      Params
      - `msg` - Input message to print.
      
      Returns i0 aka nothing. :
    function print(ptr i8 msg) => i0;
#endif
}

: include_test.cpl :
{
    #include "print_h.cpl"
    #include "string_h.cpl"

    function foo() => i0;
    
    function bar() => i0 {
        foo();
    }

    function foo() => i0 {
        print("Hello world!\n");
    }

    start(i64 argc, ptr u64 argv) {
        bar();
        exit 0;
    }
}
```

After the PP, we will get a new form of the code:

```cpl
{
#line 0 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/print_h.cpl"
#line 0 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/string_h.cpl"    
    function strlen(ptr i8 s) => i64;
#line 4 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/print_h.cpl"
    function print(ptr i8 msg) => i0;
#line 2 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/include_test.cpl"
#line 0 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/string_h.cpl"
#line 3 "/Users/nikolaj/Documents/Repositories/CordellCompiler/tests/dummy_data/include_test.cpl"

    function foo() => i0;
    
    function bar() => i0 {
        foo();
    }

    function foo() => i0 {
        print("Hello world!\n");
    }

    start(i64 argc, ptr u64 argv) {
        bar();
        exit 0;
    }
}
```

The `line` directive here serves only one purpose: it tells the tokenizer where we are.

----------------------------------------

## LiS message
<div class="change-date">Date: 2026-01-05</div>
The `lis` statement now accepts a message for convenient debugging.

```cpl
lis;                 : <= Old, but still recognizable by the compiler :
lis "Debug line #1"; : <= New :
```

These messages are placed in the final `.asm` file. With `gdb`, the decompiled source file shows the line where execution stops. The produced `.asm` file includes comments as shown below:

```asm
mov rax, 1
int3 ; Debug line #1
```

## neg, ref and dref now have their own parser
<div class="change-date">Date: 2026-01-04</div>
Previously, these statements were handled as flags on tokens. That mechanism was inconvenient in cases like the ones below:

```cpl
i32 a = not (c + b);
dref (a + 0x7C00) = 0xDEADBEEF;
```

Now these statements have their own AST and HIR handlers.

## Explicit casting is here!
<div class="change-date">Date: 2026-01-03</div>
The CPL language now supports cast operations. </br>
For instance:

```cpl
i32 a = 10 as i32;
i32 b = 10 as i32;
u8 c = (a + b) as u8;
```

## Additional operators
<div class="change-date">Date: 2026-01-03</div>
Implemented the following operators:

| Operation        | Example   |
|------------------|-----------|
| `%=`             | `X %= Y`  |
| `\|=`            | `X \|= Y` |
| `&=`             | `X &= Y`  |
| `^=`             | `X ^= Y`  |

## Loop statement
<div class="change-date">Date: 2026-01-01</div>
CPL now supports the `loop` statement!

```cpl
loop {
    break;
}
```

This statement is similar to Rust's `loop`.

## Break statement
<div class="change-date">Date: 2026-01-01</div>
CPL now supports the `break` statement!

```cpl
while 1; {
    break;
}

switch 1; {
    case 1; {
        break;
    }
    default {
        break;
    }
}
```

## PTRN DSL
<div class="change-date">Date: 2025-12-12</div>
The first phase of peephole optimization is now fully generated by the `PTRN` domain-specific language! Documentation is available in `src/lir/peephole/pattern_generator/README.md`. This allows us to construct complex and flexible templates for basic peephole optimization.

## String object
<div class="change-date">Date: 2025-12-04</div>
A string object was implemented for optimization purposes. This object is responsible for `char*` operations such as `strcat`, `strcmp`, `strcpy`, and so on. For better performance, `strcmp` and `strlen` support caching and hashing. The `strlen` function simply returns the cached value from the `string` object, while `strcmp` uses hashes for faster string comparison.

# Version v3.2
<div class="change-date">Date: 2025-11-27</div>
I am now working at ISP RAS as a research assistant in the Compiler Department's static analysis team. With this additional experience, I can now implement static analysis in CPL. The basic module contains the semantic entry point, plus AST and HIR folders. The AST folder contains the entry point for the AST walker and AST-walker behavior scripts. The main idea is simple: we have a walker with a linked list of actions for different node types:

```c
typedef enum {
    EXPRESSION_NODE  = 1 << 0,
    ASSIGN_NODE      = 1 << 1,
    DECLARATION_NODE = 1 << 2,
    FUNCTION_NODE    = 1 << 3,
    CALL_NODE        = 1 << 4,
    START_NODE       = 1 << 5,
    DEF_ARRAY_NODE   = 1 << 6,
    UNKNOWN_NODE     = 1 << 7,
} ast_node_type_t;
```

We can register handlers for each node-type. Each handler - different pattern matcher for code check process. Registration is simple:
```c
int SEM_perform_ast_check(ast_ctx_t* actx, sym_table_t* smt) {
    ast_walker_t walker;
    ASTWLK_init_ctx(&walker, smt);

    ASTWLK_register_visitor(ASSIGN_NODE, ASTWLKR_ro_assign, &walker);
    ASTWLK_register_visitor(DECLARATION_NODE | ASSIGN_NODE | EXPRESSION_NODE, ASTWLKR_rtype_assign, &walker);
    ASTWLK_register_visitor(DECLARATION_NODE, ASTWLKR_not_init, &walker);
    ASTWLK_register_visitor(DECLARATION_NODE, ASTWLKR_illegal_declaration, &walker);
    ASTWLK_register_visitor(FUNCTION_NODE, ASTWLKR_no_return, &walker);
    ASTWLK_register_visitor(START_NODE, ASTWLKR_no_exit, &walker);
    ASTWLK_register_visitor(CALL_NODE, ASTWLKR_not_enough_args, &walker);
    ASTWLK_register_visitor(CALL_NODE, ASTWLKR_wrong_arg_type, &walker);
    ASTWLK_register_visitor(CALL_NODE, ASTWLKR_unused_rtype, &walker);
    ASTWLK_register_visitor(DEF_ARRAY_NODE, ASTWLKR_illegal_array_access, &walker);

    ASTWLK_walk(actx, &walker);
    ASTWLK_unload_ctx(&walker);
    return 1;
}
```

The simplest handler is `ASTWLKR_ro_assign`. Here is its source code:

```c
// #define AST_VISITOR_ARGS ast_node_t* nd, sym_table_t* smt
int ASTWLKR_ro_assign(AST_VISITOR_ARGS) {
    ast_node_t* larg = nd->child;
    if (!larg) return 1;
    ast_node_t* rarg = larg->sibling;
    if (!rarg) return 1;

    if (larg->token->flags.ro) {
        SEMANTIC_ERROR(" [line=%i] Read-only variable=%s assign!", larg->token->finfo.line, larg->token->value);
        return 0;
    }

    return 1;
}
```

Also, this version of the compiler now operates with ACT, the automated commit tool. This tool is also simple, but it makes the commit section more readable and atomic.

### Caller-saving
<div class="change-date">Date: 2025-11-23</div>
Instruction selection and memory selection now have one additional step: register saving for pushing and popping all registers used by a function.

### Documentation update
<div class="change-date">Date: 2025-11-23</div>
The LIR part is now more complex than it was earlier, so I started the documentation sync process.

### Major refactoring
<div class="change-date">Date: 2025-11-23</div>
With a custom memory manager, it is much easier to fix memory leaks across the entire project. Also, to improve future code readability, I spent some time on a large refactoring pass.

### Fixes in inline function
<div class="change-date">Date: 2025-11-16</div>
The inline operation now copies not only variables, but also arrays with their elements if the array is local and placed on the stack. SSA also renames local arrays the same way it renames variables. An additional function for array symbol-table copying was implemented.

### LIR peephole [WIP]
<div class="change-date">Date: 2025-11-15</div>
Write optimization: remove unused write operations such as redundant moves.

### Array args list in HIR and LIR
<div class="change-date">Date: 2025-11-12</div>
Instead of storing array elements in the symbol table in `hir_subject` form, they are now stored directly in HIR and LIR array declarations. This allows function inlining to be performed more efficiently and simply. On the other hand, it makes it harder to work with global arrays defined in object `.data` and `.rodata` sections.

### DFG location
<div class="change-date">Date: 2025-11-10</div>
DFG calculation (`IN`, `OUT`, `DEF`, and `USE`) moved from HIR to LIR after instruction selection. The main idea is to preserve registers from rewriting by creating additional interference from temporary variables precolored with the registers used in an operation.

### Constant propagation
<div class="change-date">Date: 2025-11-09</div>
Constant propagation is based on DAG and updates data in the variable symbol table. It works only with constants and numbers. It propagates constants through:
- Arithmetic
- Conversion
- Copying

### LIR peephole optimization
<div class="change-date">Date: 2025-11-09</div>

| Original Instruction        | Optimized Instruction | Explanation |
|-----------------------------|-----------------------|-------------|
| `mov rax, 0`                | `xor rax, rax`        | Zeroes the register without writing to memory; resets flags; usually faster and smaller than `mov`. |
| `sub rax, rax`              | `xor rax, rax`        | Equivalent zeroing; `xor` is generally preferred. |
| `add rax, rax`              | `shl rax, 1`          | Multiply by 2 using shift; can be cheaper than `add` on some CPUs. |
| `imul rax, imm_power_of_2`  | `shl rax, log2(imm)`  | Multiplication by power of 2 replaced with shift. |
| `cmp rax, 0`                | `test rax, rax`       | `test` sets flags like `cmp` but is often cheaper. |
| `mov rax, rax`              | remove                | NOP instruction; useless. |
| `add rax, 0`, `sub rax, 0`  | remove                | Adding zero is a no-op. |
| `imul rax, 1`, `div rax, 1` | remove                | Multiplying by 1 is a no-op. |

### Instruction Planning
<div class="change-date">Date: 2025-11-09</div>
Instruction planning creates a DAG for each basic block, then reorders some instructions depending on target information. Target information is a special structure for the target CPU architecture and machine. For simplicity, I made several Python scripts in the `src/lir/instplan` directory: `build_targinfo.py` and `read_targinfo.py`.

### Regallocation
<div class="change-date">Date: 2025-11-02</div>
Register allocation moved from the HIR level to the LIR level. I plan to add support for both linear-scan and graph-based register allocation. This move allows me to use `rdx`, `rdi`, `rsi`, and similar registers without fear of rewriting them; now I can precolor variables and link them to specific registers, such as ABI registers in function calls.

### Instruction selection
<div class="change-date">Date: 2025-11-02</div>
The module for instruction selection, namely the template section, was implemented. In a few words: this module lowers the abstraction closer to the target machine. Example below.

```lirv1
fn strlen(i8* s) -> i64
{
    %16 = ldparam();
    {
        %18 = %17;
        %19 = %16;
        kill(cnst: 0);
        kill(cnst: 1);
        %17 = num: 0;
        %9 = num: 1 as u64;
        %12 = num: 1 as u64;
        lb10:
        %5 = *(%19);
        %7 = %5 as i64;
        %6 = %7 > num: 10;
        cmp %6, cnst: 0;
        je lb11;
        jmp lb12;
        lb11:
        {
            %8 = %19 / %9;
            %20 = %8;
            %11 = %18 as u64;
            %10 = %11 % %12;
            %13 = %10 as i64;
            %21 = %13;
        }
        %18 = %21;
        %19 = %20;
        jmp lb10;
        lb12:
        return %18;
    }
}
```

This transforms into code with ABI support and specific machine registers (`rax`, `rbx`, `rcx`):

```lirv2
fn strlen(i8* s) -> i64
{
    %16 = rdi;
    {
        %18 = %17;
        %19 = %16;
        kill(cnst: 0);
        kill(cnst: 1);
        %17 = num: 0;
        %9 = num: 1 as u64;
        %12 = num: 1 as u64;
        lb10:
        %5 = *(%19);
        %7 = %5 as i64;
        rax = %7;
        rbx = num: 10;
        cmp rax, rbx;
        setg al;
        %6 zx= al;
        cmp %6, cnst: 0;
        je lb11;
        jmp lb12;
        lb11:
        {
            rax = %19;
            rbx = %9;
            rax = rax / rbx;
            %8 = rax;
            %20 = %8;
            %11 = %18 as u64;
            rax = %11;
            rbx = %12;
            rdx = rdx ^ rdx;
            rdx = rax % rbx;
            %10 = rdx;
            %13 = %10 as i64;
            %21 = %13;
        }
        %18 = %21;
        %19 = %20;
        jmp lb10;
        lb12:
        return %18;
    }
}
```

# Version v3.1
<div class="change-date">Date: 2025-11-01</div>
New LIR level. Instead of straightforward LIR generation, this is now another 3AC level suitable for instruction selection and instruction planning. Also, instead of only graph-coloring register allocation, this level supports both linear-scan and graph-coloring allocation.

----------------------------------------

### Inlining 
<div class="change-date">Date: 2025-11-01</div>
A function is inlined if it reaches a score higher than 2 points.

```c
static int _inline_candidate(cfg_func_t* f, cfg_block_t* pos) {
    if (!f) return 0;
    int score = 0;

    if (
        pos->type == CFG_LOOP_BLOCK ||
        pos->type == CFG_LOOP_LATCH
    ) score += 2;

    int block_count = list_size(&f->blocks);

    if (block_count <= 2)       score += 3;
    else if (block_count <= 5)  score += 2;
    else if (block_count <= 10) score += 1;
    else if (block_count > 15)  score -= 2;
    
    return score > 2;
}
```

### TRE (tail recursion elimination)
<div class="change-date">Date: 2025-11-01</div>
The TRE implementation performs recursion elimination if the block after the recursive call is a terminator block without successors.

### IG fix
<div class="change-date">Date: 2025-11-01</div>
The interference graph is now calculated with `IN`, `DEF`, and `OUT` instead of only `DEF` and `OUT`, according to [this](https://courses.cs.cornell.edu/cs4120/2022sp/notes/regalloc/index.html) article.

### AST opt deadfunc
<div class="change-date">Date: 2025-11-01</div>
Dead-function elimination moved from the AST level to the HIR level, based on the call graph.

### SSA LICM optimization
<div class="change-date">Date: 2025-11-01</div>
Redundant calculations, except basic induction calculations, are now moved from the loop body to the loop preheader.

### CFG BB generation changed
<div class="change-date">Date: 2025-11-01</div>
The previous version of BB generation included complex `if` operations without support for two jumps, so leaders from the Dragon Book algorithm worked incorrectly. Now there are no `IFLWR`, `IFGRT`, or similar operations, only `IFOP2`.

### LIR generation based on CFG instead raw HIR
<div class="change-date">Date: 2025-11-01</div>
The LIR generator now works only with CFG data instead of a raw HIR list. It no longer produces only a raw LIR list; it also produces updated metadata for basic blocks in the CFG, including LIR-list entry and exit points for the assembly generator.

### Constant propagation
<div class="change-date">Date: 2025-11-09</div>
`HIR_DAG_sparse_const_propagation` was implemented. There are also new types for numbers and constants: constants and numbers for f/u/i 64/32/16/8.

### Debug features of CPL
<div class="change-date">Date: 2025-11-01</div>
An additional instruction called `lis` was added. Interesting abbreviation, isn't it? Is it `LinearIsStop`? Or `LiveInputStage`? Never mind. It is used for setting breakpoints in code. For example:

```cpl
start() {
    i32 a = 10;
    arr b[123, f64];
    lis; : <- Breakpoint :
    exit 1;
}
```

To use it, run the executable with a debugging tool such as `gdb` or `lldb`.

# Version v3
<div class="change-date">Date: 2025-10-21</div>
This is the third version of this compiler. A full structural transformation was performed: from the `token` -> `AST` -> `ASM` structure, which had not changed since the first version, to the `token` -> `AST` -> `HIR` (`CFG` -> `SSA` -> `DAG` -> `CFG`) -> `RA` -> `LIR` -> `ASM` structure. During development of this version, the `changelog` section was also created on 2025-10-20. Additionally, this version is the `optimization-implementation` version. List of implemented optimizations:
- HIR
    - Constant propagation
    - Constant folding
    - LICM
- LIR
    - MOV optimization

----------------------------------------

# Version v2
<div class="change-date">Date: 2025-09-01</div>
This is the second version of this compiler, at least the stable working version as of 2025-10-20. The main features are a full refactoring of the `token` part and AST generation. Cleanup was also performed, and basic `LIR` was implemented. The main improvement was the syntax of the `CP-language`: the grammar was improved.

```cplv2
extern exfunc printf;
function itoa(ptr i8 buffer, i32 dsize, i32 num) => i32 {
    i32 index = dsize - 1;
    i32 tmp = 0;

    i32 isNegative = 0;
    if num < 0; {
        isNegative = 1;
        num *= -1;
    }

    while num > 0; {
        tmp = num % 10;
        buffer[index] = tmp + 48;
        index -= 1;
        num /= 10;
    }

    if isNegative; {
        buffer[index - 1] = '-';
    }

    return 1;
}

start() {
    arr buff[32, i8];
    itoa(buff, 10, 1234567890)
    printf("%s", buff);
    exit 0;
}
```

There were some improvements in typing: now we can use Rust-like types such as `i8`, `u8`, and so on. This version also added `asm` blocks, `external` functions, `vla` arrays, and more. It was tested through the implementation of a Brainfuck interpreter.

----------------------------------------

# Version v1
<div class="change-date">Date: 2025-06-18</div>
This is the first version of this compiler. The last commit before v2 was in the middle of summer 2025. The main features of this version are a [`token` -> `AST` -> `ASM`] structure, basic NASM-syntax code generation, examples such as a Brainfuck interpreter, and so on. The most interesting part, in my opinion, is the syntax:

```cplv1
function itoa ptr buffer; int dsize; int num; {
    int index = dsize - 1;
    int tmp = 0;

    int isNegative = 0;
    if num < 0; {
        isNegative = 1;
        num = num * -1;
    }

    while num > 0; {
        tmp = num / 10;
        tmp = tmp * 10;
        tmp = num - tmp;
        tmp = tmp + 48;
        buffer[index] = tmp;
        index = index - 1;
        num = num / 10;
    }

    if isNegative == 1; {
        char minus = 45;
        buffer[index - 1] = minus;
    }

    return 1;
}
```

This version was not as friendly as the current one in terms of syntax and code style. Here is how the program body looked:

```cplv1
start
    int a = 0;
    int b = 1;
    int c = 0;
    int count = 0;
    while count < 20; {
        c = a + b;
        a = b;
        b = c;
        
        arr buffer 40 char =;
        itoa buffer 40 c;
        printf buffer 40;

        count = count + 1;
    }
exit 1;
```

Here is a sample from the old README of a function declaration:

```cplv1
function [name] [type1] arg1; [type2] arg2; ...; fstart 
: code ... :
fend [expression]; : return value :
```

Functions were able to handle only one possible output. These functions also had only one possible exit: `fend`. The situation was similar with the `if` statement:

```cplv1
if a > b; ifstart
: code ... :
ifend
```

This is also how I thought users should define arrays:

```cplv1
arr sarr 5 int = 1 2 3 4 5;
```

In summary, the first version was very simple. It only handled forward translation of tokens to assembly code through AST generation.
