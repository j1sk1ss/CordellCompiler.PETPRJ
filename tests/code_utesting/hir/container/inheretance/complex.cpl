container base {
    @[self] @[abstract]
    function init(ptr base self) -> i0;
    @[self] @[abstract]
    function destroy(ptr base self) -> i0;
}

container first::base {
    i32 a;
    @[override]
    function destroy(ptr first self) -> i0 {
        self.a = 0;
    }
    i32 b;
    @[override]
    function init(ptr first self) -> i0 {
        self.b = 0;
    }
}

container second::base {
    @[self]
    function foo(ptr second self) -> i0 {
    }
    i32 b;
    @[override]
    function init(ptr second self) -> i0 {
        self.b = 0;
    }
    i32 a;
    @[override]
    function destroy(ptr second self) -> i0 {
        self.a = 0;
    }
}

function easy(ptr base b) {
    b.init();
    b.destroy();
}

start() {
    first f;
    second s;
}


:/ OUTPUT
{
    fn destroy__first2(u8* self) -> i0
    {
        u8s* %3 = alloc;
        u8s* %3 = load_arg();
        {
            u8t* %14 = u8s* %3 + csu8 16;
            i32t* %15 = u8t* %14 as *ptr;
            i32t %16 = i8n 0 as i32;
            *(i32t* %15) = i32t %16;
        }
    }
    fn init__first3(u8* self) -> i0
    {
        u8s* %5 = alloc;
        u8s* %5 = load_arg();
        {
            u8t* %17 = u8s* %5 + csu8 24;
            i32t* %18 = u8t* %17 as *ptr;
            i32t %19 = i8n 0 as i32;
            *(i32t* %18) = i32t %19;
        }
    }
    fn foo__second4(u8* self) -> i0
    {
        u8s* %6 = alloc;
        u8s* %6 = load_arg();
        {
        }
    }
    fn init__second5(u8* self) -> i0
    {
        u8s* %8 = alloc;
        u8s* %8 = load_arg();
        {
            u8t* %20 = u8s* %8 + csu8 24;
            i32t* %21 = u8t* %20 as *ptr;
            i32t %22 = i8n 0 as i32;
            *(i32t* %21) = i32t %22;
        }
    }
    fn destroy__second6(u8* self) -> i0
    {
        u8s* %10 = alloc;
        u8s* %10 = load_arg();
        {
            u8t* %23 = u8s* %10 + csu8 32;
            i32t* %24 = u8t* %23 as *ptr;
            i32t %25 = i8n 0 as i32;
            *(i32t* %24) = i32t %25;
        }
    }
    fn easy7(u8* b)
    {
        u8s* %11 = alloc;
        u8s* %11 = load_arg();
        {
            u8t** %26 = &(u8s* %11);
            u8t %27 = u8t** %26 + csu8 0;
            use u8s* %11;
            u8t %27(u8s* %11);
            u8t** %28 = &(u8s* %11);
            u8t %29 = u8t** %28 + csu8 8;
            use u8s* %11;
            u8t %29(u8s* %11);
        }
    }
    fn _main()
    {
        {
            i0t* %30 = init__first3(u8* self) -> i0;
            i0t* %31 = destroy__first2(u8* self) -> i0;
            u8s %12 = arr_alloc(csu8 32);
            i0t* %32 = init__second5(u8* self) -> i0;
            i0t* %33 = destroy__second6(u8* self) -> i0;
            i0t* %34 = foo__second4(u8* self) -> i0;
            u8s %13 = arr_alloc(csu8 40);
        }
    }
}
/:
