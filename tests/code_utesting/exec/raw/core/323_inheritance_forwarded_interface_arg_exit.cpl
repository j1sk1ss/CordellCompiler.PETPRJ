: BLOCK_TEST :
: RUN_ASM :

container iface323 {
    @[self] @[abstract]
    function value(ptr iface323 self) -> i32;
}

container impl323::iface323 {
    @[override]
    function value(ptr impl323 self) -> i32;
}

function impl323::value(ptr impl323 self) -> i32 {
    return 42;
}

function inner323(ptr iface323 item) -> i32 {
    return item.value();
}

function outer323(ptr iface323 item) -> i32 {
    return inner323(item);
}

start() {
    impl323 item;

    exit outer323(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
