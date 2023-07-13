#pragma once

#include "arch/model/smp.h"
#include <config.h>
#include <util.h>
#include <drivers/irq/riscv_uintc.h>

#ifndef __ASSEMBLER__

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
#define CSR_SIDELEG         0x103
#define CSR_SUIST           0x1b0
#define CSR_SUIRS           0x1b1

#define IP_USIP   1 /* U-Mode software interrupt pending. */
#define IE_USIE   1 /* U-Mode software interrupt enable. */

#ifdef CONFIG_RISCV_UINTR
static inline void load_uirs(int entry, uirs_entry_t *uirs)
{
	uint64_t low, high;

	uintc_read_low(entry, &low);
	uintc_read_high(entry, &high);

	uirs->mode = low;
	uirs->hartid = low >> 16;
	uirs->irq = high;

	/* reserved bits ignored */
	uirs->reserved0 = uirs->reserved1 = 0;
}

static inline void store_uirs(int entry, uirs_entry_t *uirs)
{
	uint64_t low, high;

	low = uirs->mode | (uirs->hartid << 16);
	high = uirs->irq;

	uintc_write_low(entry, low);
	uintc_write_high(entry, high);
}

static inline void uintr_restore(tcb_t *tcb) {
	/* always deleage u-mode software interrupt */
	csr_set(CSR_SIDELEG, IE_USIE);

    /* restore user-interrupt receiver status */
    if (!tcb->tcbArch.tcbUIRecv) {
		csr_write(CSR_SUIRS, 0UL);
		csr_clear(CSR_UIE, IE_USIE);
		csr_clear(CSR_UIP, IE_USIE);
    } else {
		printf("%lu uirs restore\n", cpuIndexToID(getCurrentCPUIndex()));
        /* update uintc entry */
        uirs_entry_t uirs;
        load_uirs(tcb->tcbArch.tcbUIRecv, &uirs);
        uirs.hartid = SMP_TERNARY(cpuIndexToID(getCurrentCPUIndex()), CONFIG_FIRST_HART_ID);
        uirs.mode = 0x2;
        store_uirs(tcb->tcbArch.tcbUIRecv, &uirs);
	    csr_write(CSR_SUIRS, (1UL << 63) | tcb->tcbArch.tcbUIRecv);

        /* update notification */

		/* restore registers */
		csr_set(CSR_UIE, IE_USIE);
		csr_write(CSR_UTVEC, getRegister(tcb, UTVEC));
		csr_write(CSR_USCRATCH, getRegister(tcb, USCRATCH));
		csr_write(CSR_UEPC, getRegister(tcb, UEPC));
    }

	if (tcb->tcbArch.tcbUIRecv != 0 && tcb->tcbArch.tcbUISend.data != NULL) {
		printf("FUCK!!!!!\n");
	}

    /* restore user-interrupt sender status */
    if (!tcb->tcbArch.tcbUISend.data) {
		csr_write(CSR_SUIST, 0UL);
    } else {
		printf("%lu uist restore 0x%lx\n", cpuIndexToID(getCurrentCPUIndex()), kpptr_to_paddr(tcb->tcbArch.tcbUISend.data));
        csr_write(CSR_SUIST, (1UL << 63) | (1UL << 44) | (kpptr_to_paddr(tcb->tcbArch.tcbUISend.data) >> 0xC));
    }
}

#endif /* CONFIG_RISCV_UINTR */

#endif