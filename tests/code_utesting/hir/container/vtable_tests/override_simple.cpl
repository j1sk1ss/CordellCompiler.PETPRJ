container interface {
    @[override]
    function init(ptr interface self) -> i0;
}

function interface::init(ptr interface self) -> i0 {
}

start() {
    interface i;
}

:/ OUTPUT
{
    fn init__interface0(u8* self) -> i0
    {
        u8s* %1 = alloc;
        u8s* %1 = load_arg();
        {
        }
    }
    fn _main()
    {
        {
            i0t* %3 = init__interface0(u8* self) -> i0;
            u8s %2 = arr_alloc(csu8 8);
        }
    }
}
/: