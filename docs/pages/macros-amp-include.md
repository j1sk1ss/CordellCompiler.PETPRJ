# Macros & include

CPL has a small preprocessor. It runs before tokenization and supports:

- `#include`
- `#define`
- `#undef`
- `#ifdef`
- `#ifndef`

It also inserts `#line` markers internally so diagnostics and later compiler stages can track source locations after includes are expanded.

## Includes

Quoted includes first search relative to the current file:

```cpl
#include "print_h.cpl"
```

System-style includes search the directory passed with `-I`, then the standard
library shipped with the compiler:

```cpl
#include <stdio_h.cpl>
```

Example command:

```bash
./builds/linux-x86_64/cplc -I examples --output app main.cpl
```

The standard library location can be overridden with `CPL_INCLUDE_PATH` and
inspected with `cplc --print-stdlib-path`.

## Header pattern

Implementation file:

```cpl
function strlen(ptr i8 s) -> i64 {
    i64 l = 0;
    while dref s; {
        s += 1;
        l += 1;
    }

    return l;
}
```

Header file:

```cpl
#ifndef STRING_H_
#define STRING_H_ 0

function strlen(ptr i8 s) -> i64;

#endif
```

Use the header from another file:

```cpl
#include "string_h.cpl"

start() {
    i64 n = strlen(ref "abc");
    exit n as u8;
}
```

## Defines

`#define` performs identifier replacement. Function-like macro definitions may be parsed enough to skip their parameter list, but function-style macro expansion is not implemented as a C-compatible feature.

```cpl
#define COUNT 3

arr data[COUNT, i8] = { 'A', 'B', 'C' };
```

Conditional compilation:

```cpl
#ifndef PRINT_H_
#define PRINT_H_ 0
function print(ptr i8 s) -> i0;
#endif
```

Remove a definition:

```cpl
#undef PRINT_H_
```

### Pre-defines

Pre-processor predefines several flags:
- `CCPL_GNU64/CCPL_WINDOWS64/CCPL_MACHO64` flag
- `usize` allias to the platform's biggest unsigned type
- `isize` allias to the platform's biggest type

## Comments

CPL uses colon comments:

```cpl
: one-line comment :

:/ block comment
   across several lines /:
```

The preprocessor removes these comments before tokenization while preserving strings and character literals.
