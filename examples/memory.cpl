#define ALLOC_BUFFER_SIZE 100000
#define NULL              0 as ptr i0
#define NULL_MM_NODE      0 as ptr mm_node

container mm_node {
    u64         size;
    i8          free;
    ptr mm_node next;
}

@[section(".bss")] glob arr _mm_head[ALLOC_BUFFER_SIZE, u8];
glob ptr u8 _head = 0;

function init() -> i0 {
    _head = (ref _mm_head) as ptr u8;
    ptr mm_node curr = _head;
    curr.size = ALLOC_BUFFER_SIZE - sizeof(mm_node);
    curr.free = 1;
    curr.next = NULL_MM_NODE;
}

@[inline(always)]
function align8(i64 size) -> i64 {
    (size + 7) / 8 * 8
}

function malloc(i64 size) -> ptr i0 {
    if size <= 0; {
        return NULL;
    }

    size = align8(size);
    ptr mm_node curr = _head;

    while curr != NULL_MM_NODE; {
        if curr.free && curr.size >= size; {
            if curr.size >= size + sizeof(mm_node) + 8; {
                ptr u8 new_block_mem = curr as ptr u8;
                new_block_mem = new_block_mem + sizeof(mm_node) + size;

                ptr mm_node new_block = new_block_mem as ptr mm_node;
                new_block.size = curr.size - size - sizeof(mm_node);
                new_block.free = 1;
                new_block.next = curr.next;

                curr.size = size;
                curr.next = new_block;
            }

            curr.free = 0;

            ptr u8 result = curr as ptr u8;
            result = result + sizeof(mm_node);

            return result as ptr i0;
        }

        curr = curr.next;
    }

    return NULL;
}

function dealloc(ptr i0 mem) -> i0 {
    if not mem; {
        return;
    }

    ptr u8 block_mem = mem as ptr u8;
    block_mem = block_mem - sizeof(mm_node);

    ptr mm_node block = block_mem as ptr mm_node;
    block.free = 1;
}

@[entry("main")]
start() {
    init();
    ptr u8 buffer = malloc(10 as i64) as ptr u8;
    buffer[0] = 'M';
    buffer[1] = 'a';
    buffer[2] = 'x';
    buffer[3] = '\n';
    syscall(1, 1, buffer, 4);
    dealloc(buffer as ptr i0);
    buffer = malloc(10 as i64) as ptr u8;
    buffer[0] = 'A';
    buffer[1] = 'M';
    syscall(1, 1, buffer, 4);
    exit 0;
}