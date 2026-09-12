interface iface_bug {
    @[self] @[abstract]
    function value(ptr iface_bug self) -> i32;
}

container impl_bug::iface_bug {
    @[override]
    function value(ptr impl_bug self) -> i32;
}

function impl_bug::value(ptr impl_bug self) -> i32 {
    42
}

function call_bug(ptr iface_bug item) -> i32 {
    item.value()
}

:/ OUTPUT
{
    fn value__impl_bug1(u8* self) -> i32
    {
        u8s* %2 = alloc;
        u8s* %2 = load_arg();
        {
            use i8n 42;
        }
    }
    fn call_bug2(u8* item) -> i32
    {
        u8s* %3 = alloc;
        u8s* %3 = load_arg();
        {
            u8t** %4 = &(u8s* %3);
            u8t** %5 = u8t** %4 + csu8 0;
            u8t* %6 = *(u8t** %5);
            u8t* %7 = *(u8t* %6);
            use u8s* %3;
            u8t* %7(u8s* %3);
        }
    }
}
/: