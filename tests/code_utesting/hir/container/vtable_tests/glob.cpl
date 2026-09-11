: BUG :
@[vtable]
container base {
    @[self] function foo(ptr base self) -> i0;
}

function base::foo(ptr base self) -> i0 {
}

glob base b;

:/ OUTPUT
{
    fn foo__base0(u8* self) -> i0
    {
        u8s* %1 = alloc;
        u8s* %1 = load_arg();
        {
        }
    }
    i0t* %3 = foo__base0(u8* self) -> i0;
    u8g %2 = arr_alloc(csu8 8);
}
/: