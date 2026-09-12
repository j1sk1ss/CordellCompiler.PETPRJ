: BLOCK_TEST :
: RUN_ASM :

container iface317 {
    @[self] @[abstract]
    function value(ptr iface317 self) -> i32;
}

container first317::iface317 {
    @[override]
    function value(ptr first317 self) -> i32;
}

function first317::value(ptr first317 self) -> i32 {
    return 13;
}

container second317::iface317 {
    @[override]
    function value(ptr second317 self) -> i32;
}

function second317::value(ptr second317 self) -> i32 {
    return 29;
}

function sum317(ptr iface317 left, ptr iface317 right) -> i32 {
    return left.value() + right.value();
}

start() {
    first317 left;
    second317 right;

    exit sum317(ref left, ref right) as u8;
}

:/ OUTPUT
@exit_code=42
/:
