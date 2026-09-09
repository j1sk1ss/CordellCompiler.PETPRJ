# TODO

## V-table in containers + interfaces

To support some examples from OS-related code, we need to implement virtual table. The idea is to allocate an array of pointers in every instance of a cotainer, if it has linked functions. In a nutshell:

```cpl
cotainer std {
    i32 a;
    
    @[self]
    function init(ptr std self) -> i0;
}

function std::init(ptr std self) -> i0 {
    self.a = 0;
}

start() {
    std cnt;
    cnt.init();
    :/
    In this case, the 'cnt' actually has the next structure:
    container std {
        arr vtable[1, ptr i0];
        i32 a;
    }

    cnt.vtable[0] = std__init0;
    cnt.vtable[0]();
    /:
}
```

This will allow us to implement actual inheretence:

```cpl
@[interface]
container base {
    @[self]
    function init(ptr base self) -> i0; 
}

container std::base {
    i32 a;
    @[self]
    function init(ptr std self) -> i0;
}

function foo(ptr base smth) -> i0 {
    smth.init();
    :/
    Here we invoke the interface's function, and the difference here,
    compiler doesn't invoke 'init', it invokes the first function in
    a list of registered functions:
    smth.vtable[0]();

    And it doesn't care about the logic of this function, it just invokes
    something by a pointer and passes something what was passed by a user.
    /:
}
```

## Complete strict (strong) typing! (Completed)

We need to complete strong typing support in the compiler. To do this, the compiler should allow function types:

```cpl
ptr fn(i32,i32)i32 function;
```

I think this can be solved with a new type in the type table linked to a function from the function table. For example, it could create a dummy function:

```cpl
function __cpl_custom_typed_function(i32 _, i32 _) -> i32; :/ ID: X /:
```

Then this function will be linked to a type. With this type, we can check whether a function has the same signature. Also, if we declare two pointers with the same type, the name-generation logic will prevent template duplication. The one problem here is scopes: a new scope may create a new function. </br>
The second idea is to store this information in the type itself: keep a list of argument token types and the function return type. It would look like this:

```c
typedef struct {
    unsigned long long hash; // Fast search for an entry when we create a new one
    list_t             args;
    token_type_t       rtype;
} function_type_t;
```

At the HIR level it will be a `ptr i0` pointer with a linked type ID. This means it will not change the generated representation, but it will allow CSA to be more precise with function pointers. It may also help with lambdas:

```cpl
ptr fn(i8,i8)i8 lambda = (i8 a, i8 b) => a + b;
exit lamda(1, 1);
```

## Containers (Completed)

The idea is to create structures with a few extra features. For instance, a container should be able to hold functions with explicit `self` argument support. CPL will therefore support syntax like this:
```cpl
container storage {
    u32 wood  = 100;
    u32 steel = 100;
    u64 money;

    function sell_wood(self) -> i0 {
        self.wood -= 100;
        self.money += 100;
    }
}

start() {
    storage s;
    s.sell_wood();
}
```

In a nutshell, this code will be translated into IR like this:
```cpl
start() {
    u8* s = arrdecl(16);
    u32* tmp1 = s;
    *tmp1 = 100;
    u32* tmp2 = s + sizeof(u32);
    *tmp2 = 100;
    u64* tmp3 = s + sizeof(u32) + sizeof(u32);
    *tmp3 = 0;
    storage__sell_wood(s);
}

fn storage__sell_wood(u8* ptr) {
    u32* tmp1 = ptr;
    *tmp1 -= 100;
    u64* tmp2 = ptr + sizeof(u32);
    *tmp2 += 100;
}
```

This is the final feature that I want to add to CPL. </br>
**Important note:** Overloads and generics must work the same way, which means this code must be valid as well:
```cpl
container generic {
    function sum<T, U>(self, ptr T a, U b) -> i0 {
        return;
    }
}

start() {
    generic g;
    g.sum<i8, i8>(1, 1);
}
```

*P.S.:* Generics will not support containers. No ```sum<generic, i0>()```, etc. </br>
*P.P.S.:* No nested containers:
```cpl
container a {
    container b {

    } :/ Illegal /:
}
``` 
*P.P.P.S.:* By the way, there is a way to use a container inside another container:
```cpl
container a {
    ptr a next;
    ptr b next;
}
```

## Simple polymorphic system (Completed)

The idea is to create a placeholder type for local variables, then copy a function with the provided type. For instance, consider the function below:
```cpl
function swap<T>(ptr T a, ptr T b) -> i0 {
    T tmp = dref a;
    dref a = dref b;
    dref b = tmp;
}
```

Here `T` is an unknown type. At the AST phase, this type is treated as a `generic` type, which does not tell us anything concrete. It is also important to note that we do not generate IR for this function yet. When we encounter this code:
```cpl
swap<u8>(ref a, ref b);
```

We:
- determine which type is used for the call;
- find the called function AST node in the symbol table;
- generate IR for the function with the provided type.

This produces code like this:
```cpl
function swap1(ptr u8 a, ptr u8 b) -> i0 {
    u8 tmp = dref a;
    dref a = dref b;
    dref b = tmp;
}
swap1(ref a, ref b);
```

It should work well with the existing pipeline and overload system. It should not conflict with overloads because these features solve different problems. Generics are a convenient way to reuse the same logic for different types, while overloads make the same name available for different logic. Overloads could replace generics in some cases, but that would require writing a lot of duplicated code for each type. For instance:
```cpl
function sum<T>(T a, T b) -> T {
    return a + b;
}
sum<u8>(10, 11);
sum<f64>(10.1, 20.0);
```

To implement the same code with overloads, you would need to create two versions of `sum`:
```cpl
function sum(u8 a, u8 b) -> u8 {
    return a + b;
}
function sum(f64 a, f64 b) -> f64 {
    return a + b;
}
sum(10 as u8, 11 as u8);
sum(10.1, 20.0);
```

Meanwhile, overloads are essential for cases like this:
```cpl
function sum(u8 a, u8 b) -> u8 {
    return a + b;
}
function sum(f64 a, f64 b) -> f64 {
    return a * b - a;
}
sum(10 as u8, 11 as u8);
sum(10.1, 20.0);
```

That is why generic functions are essential and useful for systems programming. They should not take too much time to implement, they will not overcomplicate the syntax, and they are optional. </br> 
P.S.: *Actually, the compiler needs its own user-type system to support this feature. At minimum, it needs an alias system for user-defined named types, for example: typedef a i32; etc.*
