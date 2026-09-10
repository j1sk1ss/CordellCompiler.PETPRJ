: BLOCK_TEST :
: RUN_ASM :

container base316 {
    @[self] @[abstract]
    function value(ptr base316 self) -> i32;
}

container child316::base316 {
    @[override]
    function value(ptr child316 self) -> i32;
}

function child316::value(ptr child316 self) -> i32 {
    return 42;
}

function call316(ptr base316 item) -> i32 {
    return item.value();
}

start() {
    child316 item;

    exit call316(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
