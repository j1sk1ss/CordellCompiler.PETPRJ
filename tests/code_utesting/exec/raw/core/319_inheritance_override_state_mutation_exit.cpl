: BLOCK_TEST :
: RUN_ASM :

interface iface319 {
    @[self] @[abstract]
    function add(ptr iface319 self, i32 delta) -> i32;
}

container counter319::iface319 {
    i32 current;

    @[override]
    function add(ptr counter319 self, i32 delta) -> i32;
}

function counter319::add(ptr counter319 self, i32 delta) -> i32 {
    self.current += delta;
    return self.current;
}

function run319(ptr iface319 item) -> i32 {
    item.add(7 as i32);
    return item.add(12 as i32);
}

start() {
    counter319 item;
    item.current = 23;

    exit run319(ref item) as u8;
}

:/ OUTPUT
@exit_code=42
/:
