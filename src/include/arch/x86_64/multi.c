#include "multi.h"
#include <drivers/screen/fb.h>
#include <arch/x86_64/alloc.h>
#include <arch/x86_64/gdt.h>
#include <drivers/elf/elf.h>
#include <stddef.h>

char* strcpy(char* dest, const char* src) {
    char* saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

char **running_proc_names;
int *running_proc_pids;
int rpnl = 0;
int rppl = 0;
task_t* volatile current_task;
extern uint64_t read_cr3();
extern void* memset(void* s, int c, size_t n);
void small_delay() {
    for (volatile uint64_t i = 0; i < 10000000; i++) {
        // just burn cycles
    }
}
task_t* spawn_user_task(uint64_t entry_point, int id, char *name, int debug) {
    
    if(debug == 1) kprintf("[1] Starting spawn_user_task, entry=%p id=%d\n", entry_point, id);
    small_delay();
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    
    if(debug == 1) kprintf("[2] kmalloc returned %p\n", new_task);
    small_delay();if (!new_task) return NULL;

    void* kernel_stack = pmm_alloc_page();
    
    if(debug == 1) kprintf("[3] kernel_stack allocated at %p\n", kernel_stack);
    small_delay();
    void* user_stack = pmm_alloc_page();
    
    if(debug == 1) kprintf("[4] user_stack allocated at %p\n", user_stack);
    small_delay();
    memset(kernel_stack, 0, 4096);
    
    if(debug == 1) kprintf("[5] kernel_stack memset done\n");
    small_delay();
    memset(user_stack, 0, 4096);
    
    if(debug == 1) kprintf("[6] user_stack memset done\n");
small_delay();
    // 1. Prepare argv strings (high in user stack)
    uint64_t ustack_top = (uint64_t)user_stack + 4096;
    
    if(debug == 1) kprintf("[7] ustack_top = %p\n", ustack_top);
    small_delay();
    char* arg_str = (char*)(ustack_top - 64);
    
    if(debug == 1) kprintf("[8] arg_str = %p\n", arg_str);
    small_delay();
    if ((uint64_t)user_stack >= 0x1000000 && (uint64_t)user_stack < 0x1100000) {
        kprintf("ERROR: user_stack overlaps with TCC memory!\n");
        for(;;);
    }
    
    if(debug == 1) kprintf("[9] overlap check passed\n");
small_delay();
    strcpy(arg_str, name);
    
    if(debug == 1) kprintf("[10] strcpy done\n");
    small_delay();
    uint64_t* argv_array = (uint64_t*)(ustack_top - 128);
    
    if(debug == 1) kprintf("[11] argv_array = %p\n", argv_array);
    small_delay();
    argv_array[0] = (uint64_t)arg_str;
    
    if(debug == 1) kprintf("[12] argv_array[0] set\n");
    small_delay();
    argv_array[1] = 0;
    
    if(debug == 1) kprintf("[13] argv_array[1] set\n");
    small_delay();
    uint64_t argv_ptr = (uint64_t)argv_array;
    
    if(debug == 1) kprintf("[14] argv_ptr = %p\n", argv_ptr);
small_delay();
    // 2. TCC CRT0 User Stack: [argc][argv_ptr][argv[0]][argv[1]=0]
    uint64_t tcc_rsp = (uint64_t)user_stack + 4096 - 32;
    
    if(debug == 1) kprintf("[15] tcc_rsp = %p\n", tcc_rsp);
    small_delay();
    *(uint64_t*)tcc_rsp = 1;
    
    if(debug == 1) kprintf("[16] argc set\n");
    small_delay();
    *(uint64_t*)(tcc_rsp + 8) = argv_ptr;
    
    if(debug == 1) kprintf("[17] argv ptr set\n");
    small_delay();
    *(uint64_t*)(tcc_rsp + 16) = (uint64_t)arg_str;
    
    if(debug == 1) kprintf("[18] argv[0] set\n");
    small_delay();
    *(uint64_t*)(tcc_rsp + 24) = 0;
    
    if(debug == 1) kprintf("[19] argv[1] set\n");
small_delay();
    // 3. Kernel Stack Frame (160 bytes = 20 qwords)
    uint64_t frame_base = (uint64_t)kernel_stack + 4096 - 160;
    
    if(debug == 1) kprintf("[20] frame_base = %p\n", frame_base);
small_delay();
    // Software frame (offsets 0-112, indices 0-14)
    *(uint64_t*)(frame_base + 0) = 0;
    
    if(debug == 1) kprintf("[21] rax set\n");
    small_delay();
    *(uint64_t*)(frame_base + 8) = 0;
    
    if(debug == 1) kprintf("[22] rbx set\n");
    small_delay();
    *(uint64_t*)(frame_base + 16) = 0;
    
    if(debug == 1) kprintf("[23] rcx set\n");
    small_delay();
    *(uint64_t*)(frame_base + 24) = 0;
    
    if(debug == 1) kprintf("[24] rdx set\n");
    small_delay();
    *(uint64_t*)(frame_base + 32) = 0;
    
    if(debug == 1) kprintf("[25] rbp set\n");
    small_delay();
    *(uint64_t*)(frame_base + 40) = 1;
    
    if(debug == 1) kprintf("[26] rdi set\n");
    small_delay();
    *(uint64_t*)(frame_base + 48) = argv_ptr;
    
    if(debug == 1) kprintf("[27] rsi set\n");
    small_delay();
    *(uint64_t*)(frame_base + 56) = 0;
    
    if(debug == 1) kprintf("[28] r8 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 64) = 0;
    
    if(debug == 1) kprintf("[29] r9 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 72) = 0;
    
    if(debug == 1) kprintf("[30] r10 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 80) = 0;
    
    if(debug == 1) kprintf("[31] r11 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 88) = 0;
    
    if(debug == 1) kprintf("[32] r12 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 96) = 0;
    
    if(debug == 1) kprintf("[33] r13 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 104) = 0;
    
    if(debug == 1) kprintf("[34] r14 set\n");
    small_delay();
    *(uint64_t*)(frame_base + 112) = 0;
    
    if(debug == 1) kprintf("[35] r15 set\n");
small_delay();
    // Hardware frame (offsets 120-152, indices 15-19)
    *(uint64_t*)(frame_base + 120) = entry_point;
    
    if(debug == 1) kprintf("[36] rip set to %p\n", entry_point);
    small_delay();
    *(uint64_t*)(frame_base + 128) = 0x1B;
    
    if(debug == 1) kprintf("[37] cs set\n");
    small_delay();
    *(uint64_t*)(frame_base + 136) = 0x202;
    
    if(debug == 1) kprintf("[38] rflags set\n");
    small_delay();
    *(uint64_t*)(frame_base + 144) = tcc_rsp;
    
    if(debug == 1) kprintf("[39] rsp set\n");
    small_delay();
    *(uint64_t*)(frame_base + 152) = 0x23;
    
    if(debug == 1) kprintf("[40] ss set\n");
small_delay();
    // 4. Debug print frame
    
    if(debug == 1) kprintf("[41] About to print task frame debug\n");
    small_delay();
    if(debug == 1) kprintf("[multi] Task %d frame: RIP=%p RSP=%p (argc@%p)\n",        id, entry_point, tcc_rsp, *(uint64_t*)tcc_rsp);
    small_delay();
    
            if(debug == 1) kprintf("[42] Frame debug printed\n");
small_delay();
    // 5. Task setup
    new_task->stack_ptr = (void*)frame_base;
    
    if(debug == 1) kprintf("[43] stack_ptr set\n");
    small_delay();
    new_task->id = id;
    
    if(debug == 1) kprintf("[44] id set\n");
    small_delay();
    new_task->name = name;
    
    if(debug == 1) kprintf("[45] name set\n");
    small_delay();
    new_task->kernel_stack_top = (uint64_t)kernel_stack + 4096;
    
    if(debug == 1) kprintf("[46] kernel_stack_top set\n");
    small_delay();
    new_task->cr3 = read_cr3();
    
    if(debug == 1) kprintf("[47] cr3 set\n");
small_delay();
    // 6. Atomic insert to scheduler
    
    if(debug == 1) kprintf("[48] About to disable interrupts\n");
    small_delay();
    {
        task_t* _cur = current_task;
        uint64_t _cur_next = _cur ? (uint64_t)_cur->next : 0;
        if(debug == 1) kprintf("[DBG-before-cli] new_task=%p stack_ptr=%p kernel_stack_top=%p cr3=%p current_task=%p current_task->next=%p\n",
                new_task, new_task->stack_ptr, (void*)new_task->kernel_stack_top, (void*)new_task->cr3, _cur, (void*)_cur_next);
    }
    small_delay();asm volatile("cli");
    
    if(debug == 1) kprintf("[49] Interrupts disabled\n");
    small_delay();
    if (current_task == NULL) {
        
        if(debug == 1) kprintf("[50] current_task is NULL\n");
        small_delay();current_task = new_task;
        new_task->next = new_task;
    } else {
        
        if(debug == 1) kprintf("[51] current_task exists, linking\n");
        small_delay();new_task->next = current_task->next;
        
        if(debug == 1) kprintf("[52] new_task->next set\n");
        small_delay();current_task->next = new_task;
        
        if(debug == 1) kprintf("[53] current_task->next set\n");
        {
            task_t* _cur = current_task;
            if(debug == 1) kprintf("[DBG-after-link] current_task=%p current_task->next=%p new_task=%p new_task->next=%p\n",
                    _cur, _cur ? _cur->next : NULL, new_task, new_task->next);
        }
    }small_delay();
    
    asm volatile("sti");
    if(debug == 1) kprintf("[DBG-after-sti] interrupts reenabled (returning from spawn)\n");
    
    if(debug == 1) kprintf("[54] Interrupts enabled\n");
small_delay();
    
    if(debug == 1) kprintf("[multi] Spawned %d '%s' at 0x%p\n", id, name, entry_point);
    small_delay();
    if(debug == 1) kprintf("[55] spawn_user_task complete!\n");
    small_delay();return new_task;
}


int* get_rpids() { return running_proc_pids; }
char** get_rnames() { return running_proc_names; }

void schedule_preemptive(uint64_t *stack_anchor) {
    if (!current_task) return;

    current_task->stack_ptr = (void*)stack_anchor;
    current_task = current_task->next;

    if (current_task->kernel_stack_top != 0) {
        update_tss_rsp0(current_task->kernel_stack_top);
    }

    uint64_t new_cr3 = current_task->cr3;
    uint64_t old_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (new_cr3 != 0 && new_cr3 != old_cr3) {
        asm volatile("mov %0, %%cr3" : : "r"(new_cr3));
    }
}

char *str_to_print = "";
uint64_t rbx_val;

void print(char *str) {
    str_to_print = str;
    asm volatile("int $0x30" : : "a"(0), "b"(str_to_print));
}

char* itoa(int64_t value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int64_t tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while (value);

    if (tmp_value < 0 && base == 10) {
        *ptr++ = '-';
    }

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

char buf[32];

void* read(char* name) {
    uint64_t size;
    asm volatile (
        "int $0x30"
        : "=a"(size) 
        : "a"((uint64_t)12), "b"(name)
    );
    itoa(size, buf, 10);
    print(buf);

    void* result = (void*)0x4000000;
    asm volatile (
        "int $0x30"
        : 
        : "a"((uint64_t)6), "b"(name), "c"(result), "d"(size+1)
        : "memory"
    );
    return result; 
}

void create_task(uint64_t addr, int pid, char *name, int debug) {
    register uint64_t r8 asm("r8") = debug;

asm volatile (
    "int $0x30"
    :
    : "a"(3),
      "b"(addr),
      "c"((uint64_t)pid),
      "d"(name),
      "r"(r8)       // r8 holds debug
    : "memory"
);

}

void tryit() {
    print("hello there\n");
    print((char*)read("./tst/tst0.txt"));
    for (;;);
}

void task_b_main() {
    print("LAUNCHD has been launched\n");

    void* tcc = read("./System/usr/bin/tcc");
    uint8_t* check_tcc = (uint8_t*)tcc;
    kprintf("TCC Magic: %x %c %c %c\n", check_tcc[0], check_tcc[1], check_tcc[2], check_tcc[3]);
    
    // allocate 1 MiB via kmalloc syscall (syscall 7)
    uint64_t tcc_size = 1024 * 1024;
    uint64_t tcc_ptr = 0;
    asm volatile (
        "int $0x30"
        : "=a"(tcc_ptr)
        : "a"((uint64_t)7), "b"(tcc_size)
        : "memory"
    );
    if (tcc_ptr == 0) {
        kprintf("ERROR: kmalloc syscall(7) failed for TCC size=%d\n", (int)tcc_size);
        for(;;);
    }
    // kmalloc returns (new_block + 1) — payload pointer after header.
    // The actual page-aligned allocation base is at (payload - sizeof(malloc_header_t)).
    uint64_t tcc_payload = tcc_ptr;
    uint64_t tcc_base = tcc_payload - sizeof(malloc_header_t);
    kprintf("KMALLOC: payload=%p raw_base=%p size=%d\n", (void*)tcc_payload, (void*)tcc_base, (int)tcc_size);

    uint64_t tcc_entry = load_elf_pie(tcc, tcc_base);
    kprintf("TCC PIE Loaded at %p. Entry point: %p\n", tcc_base, tcc_entry);
    uint8_t* check_entry = (uint8_t*)tcc_entry;
    kprintf("Bytes at entry %p: %x %x %x %x %x %x %x %x\n", 
        tcc_entry,
        check_entry[0], check_entry[1], check_entry[2], check_entry[3],
        check_entry[4], check_entry[5], check_entry[6], check_entry[7]);

    // Diagnostic: dump PMM bitmap + memory near the ELF base to catch overlaps
    pmm_debug_range(tcc_base, tcc_size);

    // Spawn TCC directly - enable debug for verbose spawn tracing
    create_task(tcc_entry, 10, "tcc.sys", 0);
    kprintf("IMAGINE");
    
    for(;;);
}
void kernel_main_loop() {
    while (1) {
    }
}

void init_multitasking() {
    // 1. Allocate arrays
    running_proc_names = kmalloc(256 * sizeof(char*));
    running_proc_pids = kmalloc(256 * sizeof(int));
    
    for(int i = 0; i < 256; i++) {
        running_proc_names[i] = "";
        running_proc_pids[i] = 0;
    }

    // 2. Setup KERNEL taskA with proper stack frame
    void* kernel_stackA = pmm_alloc_page();
    memset(kernel_stackA, 0, 4096);
    uint64_t frame_baseA = (uint64_t)kernel_stackA + 4096 - 160;
    
    // Kernel task stack frame (EXACT context_switch order)
    // Software frame (offsets 0-112)
    *(uint64_t*)(frame_baseA +  0) = 0;    // rax
    *(uint64_t*)(frame_baseA +  8) = 0;    // rbx
    *(uint64_t*)(frame_baseA + 16) = 0;    // rcx
    *(uint64_t*)(frame_baseA + 24) = 0;    // rdx
    *(uint64_t*)(frame_baseA + 32) = 0;    // rbp
    *(uint64_t*)(frame_baseA + 40) = 0;    // rdi
    *(uint64_t*)(frame_baseA + 48) = 0;    // rsi
    *(uint64_t*)(frame_baseA + 56) = 0;    // r8
    *(uint64_t*)(frame_baseA + 64) = 0;    // r9
    *(uint64_t*)(frame_baseA + 72) = 0;    // r10
    *(uint64_t*)(frame_baseA + 80) = 0;    // r11
    *(uint64_t*)(frame_baseA + 88) = 0;    // r12
    *(uint64_t*)(frame_baseA + 96) = 0;    // r13
    *(uint64_t*)(frame_baseA +104) = 0;    // r14
    *(uint64_t*)(frame_baseA +112) = 0;    // r15
    
    // Hardware frame (offsets 120-152) - KERNEL segments
    *(uint64_t*)(frame_baseA +120) = (uint64_t)&kernel_main_loop;  // RIP = kernel loop
    *(uint64_t*)(frame_baseA +128) = 0x08;                        // CS = kernel
    *(uint64_t*)(frame_baseA +136) = 0x202;                       // RFLAGS
    *(uint64_t*)(frame_baseA +144) = frame_baseA + 120;           // RSP = kernel stack frame
    *(uint64_t*)(frame_baseA +152) = 0x10;                        // SS = kernel

    // 3. Create taskA struct
    task_t* taskA = (task_t*)kmalloc(sizeof(task_t));
    taskA->stack_ptr = (void*)frame_baseA;        // ← FIXES RIP=0x0!
    taskA->cr3 = read_cr3();
    taskA->kernel_stack_top = (uint64_t)kernel_stackA + 4096;
    taskA->id = 0;
    taskA->name = "borslade.sys";
    taskA->next = taskA;
    
    running_proc_names[rpnl++] = taskA->name;
    running_proc_pids[rppl++] = taskA->id;
    current_task = taskA;

    kprintf("[multi] Kernel taskA ready at %p\n", frame_baseA);

    // 4. Spawn launchd user task
    spawn_user_task((uint64_t)task_b_main, 1, "launchd.sys", 0);
    
    kprintf("[multi] Multitasking initialized - ready for timer interrupts!\n");
}

