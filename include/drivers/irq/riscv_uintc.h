#pragma once

#include <config.h>
#include <types.h>
#include <arch/model/smp.h>
#include <arch/machine/plic.h>

#define UINTC_WIDTH 32

#if !defined(CONFIG_PLAT_QEMU_RISCV_VIRT)
//#error "check if this platform supports a UINTC"alignas
#endif

/* User Trap Setup */
#define CSR_USTATUS         0x000
#define CSR_UIE             0x004
#define CSR_UTVEC           0x005

/* User Trap Handling */
#define CSR_USCRATCH        0x040
#define CSR_UEPC            0x041
#define CSR_UCAUSE          0x042
#define CSR_UTVAL           0x043
#define CSR_UIP             0x044

/* User Interrupt Status */
#define CSR_SUIST           0x1b0
#define CSR_SUIRS           0x1b1
#define CSR_SUICFG          0x1b2

/* S Mode CSR*/
#define CSR_SIDELEG         0x103
#define IRQ_U_SOFT          0x0
#define IE_USIE		((0x1ul) << IRQ_U_SOFT)

#define __ASM_STR(x)	#x

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
			      : "=r" (__v) :			\
			      : "memory");			\
	__v;							\
})

#define csr_write(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_clear(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define UINTC_PPTR_BASE UINTC_PPTR  // #TODO: UINTC_PPTR

static inline uint32_t readl(word_t addr) {
    return *((volatile uint32_t *)(addr));
}

static inline void writel(uint32_t val, word_t addr) {
    *((volatile uint32_t *)(addr)) = val;
}


static inline uint64_t readq(word_t addr) {
    return *((volatile uint64_t *)(addr));
}

static inline void writeq(uint64_t val, word_t addr) {
    *((volatile uint64_t *)(addr)) = val;
}

static inline int uintc_read_low(int idx, uint64_t *val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x8;
    *val = readq(addr);
    return 0;
}

static inline int uintc_read_high(int idx, uint64_t *val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x10;
    *val = readq(addr);
    return 0;

}

static inline int uintc_write_low(int idx, uint64_t val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x8;
    writeq(val, addr);
    return 0;
}

static inline int uintc_write_high(int idx, uint64_t val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x10;
    writeq(val, addr);
    return 0;
}

static inline int uintc_send(int idx) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH;
    writeq(0x1, addr);
    return 0;
}

static inline word_t uintc_get_current_hart_id(void)
{
    return SMP_TERNARY(
               cpuIndexToID(getCurrentCPUIndex()),
               CONFIG_FIRST_HART_ID);
}
