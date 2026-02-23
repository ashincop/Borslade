#include <klib/liballoc_all.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/multi.h>
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
char **running_proc_names;
int *running_proc_pids;
int rpnl = 0;
int rppl = 0;
task_t *volatile current_task;
extern uint64_t read_cr3();
task_t *spawn_task(uint64_t entry_point, int id, char *name, int debug, ring_t ring)
{
	task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
	if (!new_task)
		return NULL;
	void *kernel_stack = pmm_alloc_page();
	void *user_stack = pmm_alloc_page();
	memset(kernel_stack, 0, 4096);
	memset(user_stack, 0, 4096);

	// 1. Prepare argv strings (high in user stack)
	uint64_t ustack_top = ((uint64_t)user_stack + 4096) & ~0xF;
	// 3. Kernel Stack Frame (160 bytes = 20 qwords)
	uint64_t frame_base = (uint64_t)kernel_stack + 4096 - 160;
	// Software frame (offsets 0-112, indices 0-14)
	*(uint64_t *)(frame_base + 0) = 0;
	*(uint64_t *)(frame_base + 8) = 0;
	*(uint64_t *)(frame_base + 16) = 0;
	*(uint64_t *)(frame_base + 24) = 0;
	*(uint64_t *)(frame_base + 32) = 0;
	*(uint64_t *)(frame_base + 40) = 0;
	*(uint64_t *)(frame_base + 48) = 0;
	*(uint64_t *)(frame_base + 56) = 0;
	*(uint64_t *)(frame_base + 64) = 0;
	*(uint64_t *)(frame_base + 72) = 0;
	*(uint64_t *)(frame_base + 80) = 0;
	*(uint64_t *)(frame_base + 88) = 0;
	*(uint64_t *)(frame_base + 96) = 0;
	*(uint64_t *)(frame_base + 104) = 0;
	*(uint64_t *)(frame_base + 112) = 0;
	// Hardware frame (offsets 120-152, indices 15-19)
	*(uint64_t *)(frame_base + 120) = entry_point;
	*(uint64_t *)(frame_base + 128) = ring.cs;
	*(uint64_t *)(frame_base + 136) = ring.rflags;
	*(uint64_t *)(frame_base + 144) = ustack_top;
	*(uint64_t *)(frame_base + 152) = ring.ss;
	// 5. Task setup
	new_task->stack_ptr = (void *)frame_base;
	new_task->id = id;
	new_task->name = name;
	new_task->kernel_stack_top = (uint64_t)kernel_stack + 4096;
	new_task->cr3 = read_cr3();
	// 6. Atomic insert to scheduler
	asm volatile("cli");
	if (current_task == NULL) {
		current_task = new_task;
		new_task->next = new_task;
	} else {
		new_task->next = current_task->next;
		current_task->next = new_task;
	}

	return new_task;
}
int *get_rpids() { return running_proc_pids; }
char **get_rnames() { return running_proc_names; }