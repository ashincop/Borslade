#pragma once
#include "types.h"
void init_multitasking();
void i0h();
void schedule();
task_t* spawn_user_task(uint64_t entry_point, int id, char *name);
int* get_rpids();
char** get_rnames();