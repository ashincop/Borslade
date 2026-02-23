#pragma once
#include "libsched_core.h"
#include "libsched_tasks.h"
#define RING_INIT(cs_val, ss_val, rflags_val) \
    ((ring_t){                                                 \
        .cs = (uint64_t)(cs_val),                              \
        .rflags = (uint64_t)(rflags_val),                      \
        .ss = (uint64_t)(ss_val)                               \
    })

#define kernel_ring \
    RING_INIT(0x08, 0x10, 0x202) 
#define user_ring \
    RING_INIT(0x1B, 0x23, 0x202) 