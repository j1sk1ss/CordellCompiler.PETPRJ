: BLOCK_TEST :
: RUN_ASM :

interface iface318 {
    @[self] @[abstract]
    function value(ptr iface318 self) -> i32;
}

container impl318::iface318 {
    @[override]
    function value(ptr impl318 self) -> i32;
}

function impl318::value(ptr impl318 self) -> i32 {
    return 21;
}

function twice318(ptr iface318 item) -> i32 {
    return item.value() + item.value();
}

start() {
    impl318 item;

    exit twice318(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
