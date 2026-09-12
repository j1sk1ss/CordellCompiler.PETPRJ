interface base {
    @[self] @[abstract]
    function init(ptr base self) -> i0;
}

container first::base {
    @[override]
    function init(ptr first self) -> i0;
}

function first::init(ptr first self) -> i0 {
    i32 a = 1;
}

container second::base {
    @[override]
    function init(ptr second self) -> i0;
}

function second::init(ptr second self) -> i0 {
    i32 a = 2;
}

function easy(ptr base b) -> i0 {
    b.init();
}

start() {
    first f;
    second s;
}

: ONLY_THIS :

:/ OUTPUT
{
    fn init__first1(u8* self) -> i0
    {
        u8s* %2 = alloc;
        u8s* %2 = load_arg();
        {
            i32s %3 = alloc;
            i32t %10 = i8n 1 as i32;
            i32s %3 = i32t %10;
        }
    }
    fn init__second2(u8* self) -> i0
    {
        u8s* %5 = alloc;
        u8s* %5 = load_arg();
        {
            i32s %6 = alloc;
            i32t %11 = i8n 2 as i32;
            i32s %6 = i32t %11;
        }
    }
    fn easy3(u8* b) -> i0
    {
        u8s* %7 = alloc;
        u8s* %7 = load_arg();
        {
            u8t** %12 = &(u8s* %7);
            u8t %13 = u8t** %12 + csu8 0;
            use u8s* %7;
            u8t %13(u8s* %7);
        }
    }
    fn _main()
    {
        {
            i0t* %14 = init__first1(u8* self) -> i0;
            u8s %8 = arr_alloc(csu8 8);
            i0t* %15 = init__second2(u8* self) -> i0;
            u8s %9 = arr_alloc(csu8 8);
        }
    }
}
/:
