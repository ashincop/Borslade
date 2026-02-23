#include <klib/liballoc_all.h>
#include <arch/x86_64/gdt.h>
#include <drivers/elf/elf.h>
#include <drivers/ethernet/ethernet.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/screen/fb.h>
#include <launchd/launchd.h>
#include <stddef.h>
#include <utils/logging/log.h>
#include <utils/mem/mem.h>
#include <utils/misc/inline.h>
#include <utils/misc/str.h>
#include <kernel/sched/libsched_tasks.h>
#include <kernel/sched/libsched_all.h>
extern uint64_t read_cr3();

void schedule_preemptive(uint64_t *stack_anchor)
{
	if (!current_task)
		return;

	current_task->stack_ptr = (void *)stack_anchor;
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
void task_b_main() {
    launchd();
    for(;;);
}
void kernel_main_loop()
{
	while (1) {
	}
}
void init_multitasking()
{
	// 1. Allocate arrays
	running_proc_names = kmalloc(256 * sizeof(char *));
	running_proc_pids = kmalloc(256 * sizeof(int));

	for (int i = 0; i < 256; i++) {
		running_proc_names[i] = "";
		running_proc_pids[i] = 0;
	}

	// 2. Setup KERNEL taskA with proper stack frame
	void *kernel_stackA = pmm_alloc_page();
	memset(kernel_stackA, 0, 4096);
	uint64_t frame_baseA = (uint64_t)kernel_stackA + 4096 - 160;

	// Kernel task stack frame (EXACT context_switch order)
	// Software frame (offsets 0-112)
	*(uint64_t *)(frame_baseA + 0) = 0;   // rax
	*(uint64_t *)(frame_baseA + 8) = 0;   // rbx
	*(uint64_t *)(frame_baseA + 16) = 0;  // rcx
	*(uint64_t *)(frame_baseA + 24) = 0;  // rdx
	*(uint64_t *)(frame_baseA + 32) = 0;  // rbp
	*(uint64_t *)(frame_baseA + 40) = 0;  // rdi
	*(uint64_t *)(frame_baseA + 48) = 0;  // rsi
	*(uint64_t *)(frame_baseA + 56) = 0;  // r8
	*(uint64_t *)(frame_baseA + 64) = 0;  // r9
	*(uint64_t *)(frame_baseA + 72) = 0;  // r10
	*(uint64_t *)(frame_baseA + 80) = 0;  // r11
	*(uint64_t *)(frame_baseA + 88) = 0;  // r12
	*(uint64_t *)(frame_baseA + 96) = 0;  // r13
	*(uint64_t *)(frame_baseA + 104) = 0; // r14
	*(uint64_t *)(frame_baseA + 112) = 0; // r15

	// Hardware frame (offsets 120-152) - KERNEL segments
	*(uint64_t *)(frame_baseA + 120) =
	    (uint64_t)kernel_main_loop;		  // RIP = kernel loop
	*(uint64_t *)(frame_baseA + 128) = 0x08;  // CS = kernel
	*(uint64_t *)(frame_baseA + 136) = 0x202; // RFLAGS
	*(uint64_t *)(frame_baseA + 144) =
	    frame_baseA + 120;			 // RSP = kernel stack frame
	*(uint64_t *)(frame_baseA + 152) = 0x10; // SS = kernel

	// 3. Create taskA struct
	task_t *taskA = (task_t *)kmalloc(sizeof(task_t));
	taskA->stack_ptr = (void *)frame_baseA; // ← FIXES RIP=0x0!
	taskA->cr3 = read_cr3();
	taskA->kernel_stack_top = (uint64_t)kernel_stackA + 4096;
	taskA->id = 0;
	taskA->name = "borslade.sys";
	taskA->next = taskA;

	running_proc_names[rpnl++] = taskA->name;
	running_proc_pids[rppl++] = taskA->id;
	current_task = taskA;
	// 4. Spawn launchd user task
	spawn_task((uint64_t)task_b_main, 1, "launchd.sys", 0, user_ring);
	spawn_task((uint64_t)kernel_main_loop, 47, "main.sys", 0, kernel_ring);
}
