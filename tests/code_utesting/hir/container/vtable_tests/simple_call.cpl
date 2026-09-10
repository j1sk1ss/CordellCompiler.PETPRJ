@[vtable]
container std {
    @[self]
    function init(ptr std self) -> i0;
}

function std::init(ptr std self) -> i0 {
}

start() {
    std s;
    s.init();
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
            i0t* %3 = init__std0(u8* self) -> i0;
            u8s %2 = arr_alloc(csu8 8);
            u8t* %4 = &(u8s %2);
            u8t %5 = u8t* %4 + csu8 0;
            u8t* %6 = &(u8s %2);
            use u8t* %6;
            u8t %5(u8t* %6);
        }
    }
}
/: