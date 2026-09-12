: BLOCK_TEST :
: RUN_ASM :

interface iface322 {
    @[self] @[abstract]
    function mix(ptr iface322 self, i32 a, i32 b, i32 c) -> i32;
}

container weighted322::iface322 {
    @[override]
    function mix(ptr weighted322 self, i32 a, i32 b, i32 c) -> i32;
}

function weighted322::mix(ptr weighted322 self, i32 a, i32 b, i32 c) -> i32 {
    return a + b * 2 + c * 3;
}

function call322(ptr iface322 item) -> i32 {
    return item.mix(4 as i32, 7 as i32, 8 as i32);
}

start() {
    weighted322 item;

    exit call322(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
