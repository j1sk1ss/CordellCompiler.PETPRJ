container base316 {
    @[self] @[abstract]
    function value(ptr base316 self) -> i32;
}

container child316::base316 {
    @[override]
    function value(ptr child316 self) -> i32;
}

function child316::value(ptr child316 self) -> i32 {
    return 42;
}

function call316(ptr base316 item) -> i32 {
    return item.value();
}

start() {
    child316 item;

    exit call316(ref item) as u8;
}


:/ OUTPUT
{
    fn value__child3161(u8* self) -> i32
    {
        u8s* %2 = alloc;
        u8s* %2 = load_arg();
        {
            i32t %5 = i8n 42 as i32;
            return i32t %5;
        }
    }
    fn call3162(u8* item) -> i32
    {
        u8s* %3 = alloc;
        u8s* %3 = load_arg();
        {
            u8t** %6 = &(u8s* %3);
            u8t** %7 = u8t** %6 + csu8 0;
            u8t* %8 = *(u8t** %7);
            u8t* %9 = *(u8t* %8);
            use u8s* %3;
            u64t %10 = u8t* %9(u8s* %3);
            return u64t %10;
        }
    }
    fn _main()
    {
        {
            i0t* %11 = &(value__child3161(u8* self) -> i32);
            u8s %4 = arr_alloc(csu8 8);
            u8t* %13 = &(u8s %4);
            use u8t* %13;
            i32t %14 = call3162(u8t* %13);
            u8t %12 = i32t %14 as u8;
            exit u8t %12;
        }
    }
}
/: