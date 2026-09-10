@[vtable]
container simple {
    i32 a;
    function foo() -> i8 {
        10
    }

    function bar() -> i8;
    
    i32 b;

    @[self] 
    function baz(ptr simple self) -> i0;
}

function simple::bar() -> i8 {
    1
}

function simple::baz(ptr simple self) -> i0 {
    self.a = 1;
    self.b = 1;
}

start() {
    simple s;

    simple::foo();
    simple::bar();
    s.baz();
}

:/ OUTPUT
{
    fn foo__simple0() -> i8
    {
        {
            use i8n 10;
        }
    }
    fn bar__simple1() -> i8
    {
        {
            use i8n 1;
        }
    }
    fn baz__simple2(u8* self) -> i0
    {
        u8s* %3 = alloc;
        u8s* %3 = load_arg();
        {
            u8t* %5 = u8s* %3 + csu8 0;
            i32t* %6 = u8t* %5 as *ptr;
            i32t %7 = i8n 1 as i32;
            *(i32t* %6) = i32t %7;
            u8t* %8 = u8s* %3 + csu8 24;
            i32t* %9 = u8t* %8 as *ptr;
            i32t %10 = i8n 1 as i32;
            *(i32t* %9) = i32t %10;
        }
    }
    fn _main()
    {
        {
            i0t* %11 = baz__simple2(u8* self) -> i0;
            u8s %4 = arr_alloc(csu8 40);
            foo__simple0();
            bar__simple1();
            u8t* %12 = &(u8s %4);
            u8t %13 = u8t* %12 + csu8 0;
            u8t* %14 = &(u8s %4);
            use u8t* %14;
            u8t %13(u8t* %14);
        }
    }
}
/: