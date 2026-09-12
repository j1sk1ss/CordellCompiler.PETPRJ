interface smth {
    @[self]
    function foo(ptr smth self) -> i32;
}

container impl::smth {
    @[override]
    function foo(ptr impl self) -> i32;
}

function impl::foo(ptr impl self) -> i32 {
    return 42;
}

glob impl item;

glob function get() -> ptr smth {
    return ref item;
}

:/ OUTPUT
{
    fn foo__impl1(u8* self) -> i32
    {
        u8s* %2 = alloc;
        u8s* %2 = load_arg();
        {
            i32t %4 = i8n 42 as i32;
            return i32t %4;
        }
    }
    u8g %3 = arr_alloc(csu8 8);
    fn get2() -> u8*
    {
        {
            u8t* %5 = &(u8g %3);
            return u8t* %5;
        }
    }
}
/: