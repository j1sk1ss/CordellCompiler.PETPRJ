interface base {
    @[self]
    function init(ptr base self) -> i0;
}

container first::base {
    @[override]
    function init(ptr first self) -> i0 {
    }
    @[self]
    function foo(ptr first self) -> i0;
}

function first::foo(ptr first self) -> i0 {
}

function easy(ptr base b) {
    b.init();
}

start() {
    first f;
    f.init();
    f.foo();
}

:/ OUTPUT
{
    fn init__first1(u8* self) -> i0
    {
        u8s* %1 = alloc;
        u8s* %1 = load_arg();
        {
        }
    }
    fn foo__first2(u8* self) -> i0
    {
        u8s* %3 = alloc;
        u8s* %3 = load_arg();
        {
        }
    }
    fn easy3(u8* b)
    {
        u8s* %4 = alloc;
        u8s* %4 = load_arg();
        {
            u8t** %6 = &(u8s* %4);
            u8t** %7 = u8t** %6 + csu8 0;
            u8t* %8 = *(u8t** %7);
            u8t* %9 = *(u8t* %8);
            use u8s* %4;
            u8t* %9(u8s* %4);
        }
    }
    fn _main()
    {
        {
            i0t* %10 = &(init__first1(u8* self) -> i0);
            u8s %5 = arr_alloc(csu8 8);
            u8t* %11 = &(u8s %5);
            u8t* %12 = u8t* %11 + csu8 0;
            u8t* %13 = *(u8t* %12);
            u8t* %14 = &(u8s %5);
            use u8t* %14;
            u8t* %13(u8t* %14);
            u8t* %15 = &(u8s %5);
            use u8t* %15;
            foo__first2(u8t* %15);
        }
    }
}
/: