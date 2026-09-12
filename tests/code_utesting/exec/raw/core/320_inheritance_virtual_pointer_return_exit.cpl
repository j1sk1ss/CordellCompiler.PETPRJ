: BLOCK_TEST :
: RUN_ASM :

container iface320 {
    @[self] @[abstract]
    function marker(ptr iface320 self) -> ptr i8;
}

container text320::iface320 {
    @[override]
    function marker(ptr text320 self) -> ptr i8;
}

function text320::marker(ptr text320 self) -> ptr i8 {
    return ref "x*";
}

function read320(ptr iface320 item) -> i32 {
    ptr i8 marker = item.marker();
    return marker[1] as i32;
}

start() {
    text320 item;

    exit read320(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
