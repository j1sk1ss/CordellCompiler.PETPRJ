: BLOCK_TEST :
: RUN_ASM :

interface root321 {
    @[self] @[abstract]
    function value(ptr root321 self) -> i32;
}

interface middle321::root321 {
}

container impl321::middle321 {
    @[override]
    function value(ptr impl321 self) -> i32;
}

function impl321::value(ptr impl321 self) -> i32 {
    return 42;
}

function call321(ptr root321 item) -> i32 {
    return item.value();
}

start() {
    impl321 item;

    exit call321(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
