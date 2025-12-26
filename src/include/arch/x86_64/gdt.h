#pragma once
#include "types.h"
void install_gdt();
void update_tss_rsp0(uint64_t new_rsp);