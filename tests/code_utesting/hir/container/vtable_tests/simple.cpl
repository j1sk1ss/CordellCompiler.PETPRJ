: REWRITE :
@[vtable]
container std {
    @[self]
    function init(ptr std self) -> i0;
}

function std::init(ptr std self) -> i0 {
}

start() {
    std s;
}

:/ OUTPUT
{
    fn init__std0(u8* self) -> i0
    {
        u8s* %1 = alloc;
        u8s* %1 = load_arg();
        {
        }
    }
    fn _main()
    {
        {
            u8s %2 = arr_alloc(csu8 0);
        }
    }
}
/: