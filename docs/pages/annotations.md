# Annotations

Annotations extend the small core syntax without adding many dedicated keywords. They resemble Java and Rust annotations and are written before the construct they affect:

```cpl
:/ entry affects the function /:
@[entry("_main")] function main() -> i0 {
    exit 0;
}
```

The parser accepts `@[name]`, `@[name(value / variable)]` and `@[name(value / variable, value / variable)]`.

## Available annotations

| Annotation                                    | Applies to                                                       | Meaning                                                                                            |
|-----------------------------------------------|------------------------------------------------------------------|----------------------------------------------------------------------------------------------------|
| `@[register(N)]`                              | variable declaration                                             | bind the variable to a target register index                                                       |
| `@[poparg]`                                   | variable declaration in a variadic context                       | read the next variadic argument into this declaration                                              |
| `@[not_null]`                                 | variable declaration                                             | mark a variable as an object that cannot store 0; CSA only, does not change compilation            |
| `@[popreg(N)]`                                | variable declaration                                             | load a value form a register `N` to a variable                                                     |
| `@[volatile]`                                 | variable declaration                                             | preserve variable from drop by compiler in result of an optimization                               |
| `@[align(N)]`                                 | variable, array, or container                                    | request memory/container alignment                                                                 |
| `@[section("name")]`, `@[section("name", N)]` | global or read-only variable, global array, function, or `start` | place the symbol into a named section; optional `N` sets section alignment                         |
| `@[entry]`, `@[entry("name")]`                | function or `start`                                              | mark the function as the program entry; without `name`, the configured entry symbol is used        |
| `@[naked]`                                    | function or `start`                                              | suppress normal entry/exit routines                                                                |
| `@[nosection]`                                | global function                                                  | place the function into the configured no-section bucket                                           |
| `@[inline]`                                   | function                                                         | increase the inliner preference                                                                    |
| `@[inline(always)]`                           | function                                                         | force the inline decision toward always inline                                                     |
| `@[inline(never)]`                            | function                                                         | force the inline decision toward never inline                                                      |
| `@[inline(model)]`                            | function                                                         | use the model-based inline mode                                                                    |
| `@[only_body]`                                | function                                                         | emit only the function body, without the normal label/export wrapper                               |
| `@[abi]`                                      | function                                                         | mark the function as ABI-compatible                                                                |
| `@[weak]`                                     | function                                                         | mark the function as a weak symbol                                                                 |
| `@[vname("symbol")]`                          | function                                                         | use an explicit backend/linker-visible symbol name without marking the function as the entry point |
| `@[self]`                                     | container function                                               | mark the function as an explicit-self method for container call rewriting                          |
| `@[like_c]`                                   | container                                                        | use C-like field layout handling instead of the requested CPL alignment value                      |
| `@[union]`                                    | container                                                        | lay out all fields at offset zero and allocate enough memory for the largest field                 |
| `@[no_fall]`                                  | `switch`                                                         | make switch cases behave as if they end with `break`                                               |
| `@[straight]`                                 | `switch`                                                         | force linear switch selection                                                                      |
| `@[counter(N, STP)]`                          | `loop`                                                           | generate a counted loop where 'STP' is optional                                                    |
| `@[hot]`                                      | `if`                                                             | make the false branch cold for layout                                                              |
| `@[cold]`                                     | `if` or switch `case`                                            | make the true branch, or the annotated case, cold for layout                                       |
| `@[not_lazy]`                                 | logical expression                                               | evaluate both sides of `&&` or `\|\|`                                                              |

## Entry, naked, sections

```cpl
@[entry("_main")] function main() -> i0 {
    exit 0;
}

@[naked] start() {
    asm() {
        "ret"
    }
}

@[section(".my_text", 16)] function helper() -> i0 {
    return;
}
```

Use `@[naked]` only for code that fully controls its own prologue, epilogue, and exit behavior. Default sections are target/config dependent. The CLI exposes `--ro-section`, `--glob-section`, and `--code-section`. If several symbols are placed in the same section with an alignment argument, the section table keeps the maximum requested alignment.

`@[nosection]` is currently handled for functions:

```cpl
@[nosection] glob function tss_flush() -> i0 {
    asm() {
        "mov ax, 0x28",
        "ltr ax"
    }
}
```

## Data layout

```cpl
@[align(16)] glob i32 value = 1;
@[section(".my_data")] glob i32 other = 2;

@[align(1)]
container packed {
    i8  a;
    i16 b;
}

@[like_c]
container c_layout {
    i8  tag;
    i64 value;
}

@[union]
container word_view {
    u32 word;
    arr bytes[4, u8];
}
```

## Function hints and symbols

```cpl
@[inline(always)]
function add(i64 a, i64 b) -> i64 {
    return a + b;
}

@[weak]
@[abi]
function external_hook() -> i0;

@[vname("_printf")]
@[abi]
extern function printf(ptr i8 fmt, ...) -> i32;
```

`@[inline]` without an option is a soft preference. Supported options are `always`, `never`, and `model`. </br>
`@[abi]`, `@[weak]`, and `@[vname("symbol")]` are low-level symbol/interop flags used by the function table and backend path. `@[entry("symbol")]` also gives a function a backend-visible name, but it additionally marks that function as the program entry point. Use `@[vname("symbol")]` when only the emitted symbol name should change.

## Container self methods

Use `@[self]` when a container function should receive the object being called on. The function must declare an explicit first parameter for that receiver, usually `ptr <container> self`.

```cpl
container counter {
    i32 value;

    @[self]
    function add(ptr counter self, i32 delta) -> i0 {
        self.value += delta;
    }
}

start() {
    counter c;
    c.value = 10;
    c.add(7);

    exit c.value as u8;
}
```

The call `c.add(7)` is lowered as a normal function call where `ref c` is passed as the explicit `self` argument.

## Switch annotations

`@[no_fall]` removes the need to write `break` at the end of every case:

```cpl
@[no_fall]
switch code; {
    case 'A'; { putc('A'); }
    case 'B'; { putc('B'); }
    default   { putc('?'); }
}
```

`@[straight]` asks the compiler to use a linear search instead of the default binary-search-style generation:

```cpl
@[straight] @[no_fall]
switch code; {
    case 1; { putc('1'); }
    default { putc('?'); }
}
```

## Branch layout

`@[hot]` and `@[cold]` are layout hints for `if` statements:

```cpl
@[hot] if likely; {
    putc('H');
}
else {
    putc('C');
}
```

On `if`, `@[hot]` makes the false branch cold; `@[cold]` makes the true branch cold. On `switch`, `@[cold]` can be attached to a case body.

## Counted loop

```cpl
@[counter(10)] loop putc('x');
@[counter(10, 1)] loop putc('x');

i32 a = 10;
i32 b = 0;
@[counter(a, b)] loop putc('x');
```

## Logical evaluation

By default, logical operators are lazy. Attach `@[not_lazy]` to force both sides to be generated:

```cpl
if (@[not_lazy] left() && right()); {
    putc('y');
}
```

## Register and poparg

`@[register(N)]` binds a variable to a target register index:

```cpl
#include <regs_h.cpl>
@[register(RAX)] i64 value = 10;
```

`@[popreg(N)]` loads to a variable data from a register:

```cpl
#include <regs_h.cpl>
@[popreg(RAX)] usize value;
```

`@[poparg]` reads arguments from the current variadic call context:

```cpl
function take(...) -> i0 {
    @[poparg] i64 first;
    @[poparg] i64 second;
}

start(...) {
    @[poparg] i64 argc;
    @[poparg] ptr ptr i8 argv;
    exit 0;
}
```

Both annotations are low-level and target-sensitive.
