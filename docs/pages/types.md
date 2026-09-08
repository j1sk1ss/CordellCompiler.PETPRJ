# Types

CPL's type system is intentionally small. The current language core consists of primitive types, pointers, strings, arrays, and user-defined value-layout containers. CPL does not support classes, enums, traits, modules, or automatic memory-safety guarantees.

## Primitive types

| Type | Meaning |
|---|---|
| `i8`, `i16`, `i32`, `i64` | signed integers |
| `u8`, `u16`, `u32`, `u64` | unsigned integers |
| `usize`, `isize`          | unsigned and signed maximum sized type (used when preprocessor is enabled) |
| `f32`, `f64` | floating-point values |
| `i0` | void-like type, mainly for function return types and `ptr i0` |

Examples:

```cpl
i8  c      = 'A';
u8  b      = 0xFF;
i32 x      = 10 as i32;
i64 y      = 123456;
f64 z      = 0.25;
usize umax = 0x01132;
isize imax = 0xDEADBEEF;
```

## Static typing and casts

CPL is statically typed, but the compiler may insert implicit widening conversions. Narrowing conversions must be written explicitly with `as`.

```cpl
i64 wide  = 10;
i8 narrow = wide as i8;
```

`as` is also useful when selecting an overloaded function:

```cpl
function put(i8 x) -> i0;
function put(i64 x) -> i0;

put(1 as i8);
```

## Boolean-like logic

CPL has no dedicated boolean type. Conditions use primitive values:

- `0` is false;
- any non-zero value is true.

## Pointers

Use `ptr` to declare a pointer, `ref` to take an address, and `dref` to read through a pointer.

```cpl
i32 value = 123;
ptr i32 p = ref value;
i32 copy = dref p;
```

Indexed pointer access is also supported:

```cpl
i32 first = p[0];
```

Pointers can be chained:

```cpl
ptr i32 p = ref value;
ptr ptr i32 pp = ref p;
```

`ptr i0` is commonly used for untyped pointers and function pointers.

## Arrays

Arrays use the syntax `arr name[length, type]`.

```cpl
arr a[3, i8] = { 'A', 'B', 'C' };
i8 second = a[1];
```

Arrays can contain primitive types, pointers, containers and arrays:

```cpl
arr row1[3, i32] = { 1, 2, 3 };
arr row2[3, i32] = { 4, 5, 6 };
arr rows[2, ptr i32] = { ref row1, ref row2 };
arr matrix[4, arr[4, i32]];
i32 x = rows[1][0];
```

Global and read-only arrays are placed into target-dependent sections. Local arrays are allocated in function-local storage. </br>
To store a string, use this form:

```cpl
arr msg[0, i8] = "Hello world!";
```

Initializer lists handle string literals as static data references. This is different from ordinary stack/runtime expressions: when a function expects `ptr i8`, write `ref "..."`; inside a global array or container initializer, write the string literal without `ref`.

## Containers

Containers are user-defined value-layout types, similar in spirit to C structs. They can contain primitive fields, pointer fields, arrays, other containers, and functions. A container can also be marked with `@[union]`, in which case its fields share storage like a C union.

```cpl
container point {
    i32 x;
    i32 y;
}

container line {
    point a;
    point b;
    arr color[4, i8];
}
```

Local container variables are ordinary function-local storage:

```cpl
line l;
l.a.x = 1;
l.b.y = 2;
l.color[0] = 255 as i8;
```

Container functions can be regular functions, generic functions, or self-style methods with the `@[self]` annotation and an explicit pointer receiver. Implementations can live inside the container body or be provided later with `container_name::function_name`. See [Containers](#/pages/containers) for the full syntax and examples.

```cpl
container message {
    ptr i8 text;
    arr inline_text[8, i8];
}

glob message msg = { "Hello", "World!" };

function print(ptr i8 text) -> i0;

start() {
    print(ref "Hello from stack");
}
```

In the initializer above, `text` receives a pointer to the generated static string, while `inline_text` is filled byte-by-byte because its field type is `arr [N, i8]`.

When indexing an array whose element type is itself an array or a container, the indexed expression is already a reference to that element storage. For example, if `items` is an array of containers, `items[0]` can be passed where `ptr item` is expected; `ref items[0]` is not needed.

## `glob`, `ro`, and `extern`

`glob` makes a symbol global:

```cpl
glob i32 counter = 0;
glob function exported() -> i0;
```

`ro` marks data as read-only:

```cpl
ro i32 answer = 42;
```

`extern` declares a symbol provided by another object or library:

```cpl
extern function puts(ptr i8 s) -> i32;
extern i32 errno;
```

External symbols must be linked correctly by the final toolchain.
