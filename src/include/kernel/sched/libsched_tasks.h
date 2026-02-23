#pragma once
#include <stdint.h>
typedef struct {
    uint64_t cs;
    uint64_t ss;
    uint64_t rflags;
} ring_t;
typedef struct task {
	void *stack_ptr;   // Offset 0: Saved RSP
	uint64_t cr3;	   // Offset 8: Page Table base
	struct task *next; // Offset 16: Next in list
	int id;		   // Offset 24: Debug ID
	uint64_t
	    kernel_stack_top; // Offset 32: Landing pad for Ring 3 -> Ring 0
	char *name;
} task_t __attribute__((packed));
task_t *spawn_task(uint64_t entry_point, int id, char *name, int debug, ring_t ring);
int *get_rpids();
char **get_rnames();
extern char **running_proc_names;
extern int *running_proc_pids;
extern int rpnl;
extern int rppl;
extern task_t *volatile current_task;