#include <utils/logging/log.h>
#include <uacpi/kernel_api.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <arch/x86_64/alloc.h>
#define host "com.uACPI.all"
/* --- 1. Memory Management --- */

void* uacpi_kernel_alloc(uacpi_size size) {
    return kmalloc(size);
}

void uacpi_kernel_free(void* mem) {
    // Memory is leaked as requested
    (void)mem;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    // Returns virtual address. Identity mapping assumed for now.
    (void)len;
    return (void*)addr; 
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    (void)addr; (void)len;
}

/* --- 2. Synchronization & Tasks --- */

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)1; // Constant ID since we use spinlocks for isolation
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    uint32_t* lock = uacpi_kernel_alloc(sizeof(uint32_t));
    if (lock) *lock = 0;
    return (uacpi_handle)lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle h) { (void)h; }

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle h) {
    volatile uint32_t* lock = (volatile uint32_t*)h;
    uacpi_cpu_flags flags;
    // Disable interrupts to stop the preemptive scheduler
    asm volatile("pushfq; pop %0; cli" : "=rm"(flags) : : "memory");
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) { asm volatile("pause"); }
    return flags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle h, uacpi_cpu_flags flags) {
    volatile uint32_t* lock = (volatile uint32_t*)h;
    __atomic_clear(lock, __ATOMIC_RELEASE);
    asm volatile("push %0; popfq" : : "rm"(flags) : "memory");
}

uacpi_handle uacpi_kernel_create_mutex(void) { return uacpi_kernel_create_spinlock(); }
void uacpi_kernel_free_mutex(uacpi_handle h) { (void)h; }

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle h, uacpi_u16 timeout) {
    (void)timeout; // We ignore timeout and spin because we can't yield
    uacpi_kernel_lock_spinlock(h); 
    return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle h) {
    // Note: Manual unlock since Mutex API doesn't provide flags
    volatile uint32_t* lock = (volatile uint32_t*)h;
    __atomic_clear(lock, __ATOMIC_RELEASE);
    asm volatile("sti"); 
}

/* --- 3. Events & Work --- */

uacpi_handle uacpi_kernel_create_event(void) {
    uint64_t* counter = uacpi_kernel_alloc(sizeof(uint64_t));
    if (counter) *counter = 0;
    return (uacpi_handle)counter;
}

void uacpi_kernel_free_event(uacpi_handle h) { (void)h; }

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle h, uacpi_u16 timeout) {
    volatile uint64_t* counter = (volatile uint64_t*)h;
    uint32_t spin = timeout * 10000;
    while (*counter == 0) {
        if (timeout != 0xFFFF && spin-- == 0) return UACPI_FALSE;
        asm volatile("pause");
    }
    __atomic_fetch_sub(counter, 1, __ATOMIC_RELAXED);
    return UACPI_TRUE;
}

void uacpi_kernel_signal_event(uacpi_handle h) {
    __atomic_fetch_add((volatile uint64_t*)h, 1, __ATOMIC_RELAXED);
}

void uacpi_kernel_reset_event(uacpi_handle h) { *(volatile uint64_t*)h = 0; }

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type t, uacpi_work_handler h, uacpi_handle ctx) {
    (void)t; h(ctx); return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) { return UACPI_STATUS_OK; }

/* --- 4. I/O & PCI Access --- */

uacpi_status uacpi_kernel_io_map(uacpi_io_addr b, uacpi_size l, uacpi_handle *o) {
    *o = (uacpi_handle)(uintptr_t)b; (void)l; return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle h) { (void)h; }

#define IO_FUNC(name, type, inst) \
uacpi_status uacpi_kernel_io_##name(uacpi_handle h, uacpi_size o, type *v) { \
    uint16_t p = (uint16_t)((uintptr_t)h + o); \
    asm volatile(inst " %1, %0" : "=a"(*v) : "Nd"(p)); \
    return UACPI_STATUS_OK; \
}
IO_FUNC(read8, uacpi_u8, "inb")
IO_FUNC(read16, uacpi_u16, "inw")
IO_FUNC(read32, uacpi_u32, "inl")

#define IO_WRITE(name, type, inst) \
uacpi_status uacpi_kernel_io_##name(uacpi_handle h, uacpi_size o, type v) { \
    uint16_t p = (uint16_t)((uintptr_t)h + o); \
    asm volatile(inst " %0, %1" : : "a"(v), "Nd"(p)); \
    return UACPI_STATUS_OK; \
}
IO_WRITE(write8, uacpi_u8, "outb")
IO_WRITE(write16, uacpi_u16, "outw")
IO_WRITE(write32, uacpi_u32, "outl")

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address a, uacpi_handle *o) {
    uint32_t bdf = 0x80000000 | (a.bus << 16) | (a.device << 11) | (a.function << 8);
    *o = (uacpi_handle)(uintptr_t)bdf; return UACPI_STATUS_OK;
}
void uacpi_kernel_pci_device_close(uacpi_handle h) { (void)h; }

static uint32_t pci_read(uacpi_handle h, uacpi_size o) {
    uint32_t addr = (uint32_t)(uintptr_t)h | (o & 0xFC);
    asm volatile("outl %0, %1" : : "a"(addr), "Nd"((uint16_t)0xCF8));
    uint32_t v; asm volatile("inl %1, %0" : "=a"(v) : "Nd"((uint16_t)0xCFC));
    return v;
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle h, uacpi_size o, uacpi_u8 *v) {
    *v = (pci_read(h, o) >> ((o & 3) * 8)) & 0xFF; return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read16(uacpi_handle h, uacpi_size o, uacpi_u16 *v) {
    *v = (pci_read(h, o) >> ((o & 2) * 8)) & 0xFFFF; return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read32(uacpi_handle h, uacpi_size o, uacpi_u32 *v) {
    *v = pci_read(h, o); return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write8(uacpi_handle d, uacpi_size o, uacpi_u8 v) { return UACPI_STATUS_OK; }
uacpi_status uacpi_kernel_pci_write16(uacpi_handle d, uacpi_size o, uacpi_u16 v) { return UACPI_STATUS_OK; }
uacpi_status uacpi_kernel_pci_write32(uacpi_handle d, uacpi_size o, uacpi_u32 v) { return UACPI_STATUS_OK; }

/* --- 5. Timing & Logs --- */

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    static uint64_t fake = 0; return fake += 1000;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    for (uint32_t i = 0; i < (uint32_t)usec * 1000; i++) { asm volatile("pause"); }
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    for (uacpi_u64 i = 0; i < msec; i++) { uacpi_kernel_stall(250); uacpi_kernel_stall(250); uacpi_kernel_stall(250); uacpi_kernel_stall(250); }
}

void uacpi_kernel_log(uacpi_log_level l, const uacpi_char* s) {
    // This is the "bridge" between the library and your kernel.
    // If 's' contains "[uACPI] Hello", we strip the first 7 characters.
    const char* message = s;
    
    // Check if the library is prepending the annoying string
    if (s[0] == '[' && s[1] == 'u') {
        message = s + 7; // Skip "[uACPI]" and the space
    }

    // Now use your clean, unified kprintf
    log(host, O_ALL, "%s", message);
}

/* --- 6. Firmware & Interrupts --- */

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request* r) { (void)r; return UACPI_STATUS_OK; }

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, 
    uacpi_interrupt_handler handler, 
    uacpi_handle ctx, 
    uacpi_handle *out_irq_handle
) {
    // We ignore the actual registration for now
    (void)irq; (void)handler; (void)ctx;
    
    // Return a dummy handle and success to let uACPI continue
    *out_irq_handle = (uacpi_handle)0xDEAD; 
    return UACPI_STATUS_OK; 
}


uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler h, uacpi_handle irq_h) {
    (void)h; (void)irq_h; return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out) {
    extern void* find_xsdp_from_mb2(uint64_t);
    extern uint64_t g_mbi_ptr; 
    *out = (uacpi_phys_addr)find_xsdp_from_mb2(g_mbi_ptr);
    return *out ? UACPI_STATUS_OK : UACPI_STATUS_NOT_FOUND;
}