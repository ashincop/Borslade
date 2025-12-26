#pragma once
#include <stdint.h>
void keyboard_handler(uint64_t *stack_anchor);
void keyboard_init();
void kscan(char* buf);