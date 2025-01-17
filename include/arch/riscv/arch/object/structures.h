/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2015, 2016 Hesham Almatary <heshamelmatary@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#ifndef __ASSEMBLER__
#include <config.h>
#include <assert.h>
#include <util.h>
#include <api/types.h>
#include <arch/types.h>
#include <arch/object/structures_gen.h>
#include <arch/machine/hardware.h>
#include <arch/machine/registerset.h>
#include <mode/object/structures.h>

#define tcbArchCNodeEntries tcbCNodeEntries

struct asid_pool {
    pte_t *array[BIT(asidLowBits)];
};

typedef struct asid_pool asid_pool_t;

#define ASID_POOL_PTR(r)    ((asid_pool_t*)r)
#define ASID_POOL_REF(p)    ((word_t)p)
#define ASID_BITS           (asidHighBits + asidLowBits)
#define nASIDPools          BIT(asidHighBits)
#define ASID_LOW(a)         (a & MASK(asidLowBits))
#define ASID_HIGH(a)        ((a >> asidLowBits) & MASK(asidHighBits))

#ifdef CONFIG_RISCV_UINTR
/* User Interrupt Sender Status Table Entry (UISTE) */
struct uist_entry {
	uint8_t valid;
	uint8_t reserved0;
	uint16_t send_vec;
	uint16_t reserved1;
	uint16_t uirs_index;
};
typedef struct uist_entry uist_entry_t;

/* User Interrupt Receiver Status Table Entry (UIRSE) */
struct uirs_entry {
	uint8_t mode;
	uint8_t reserved0;
	uint16_t hartid;
	uint32_t reserved1;
	uint64_t irq;
};
typedef struct uirs_entry uirs_entry_t;

/* User Interrupt Receiver Status Index */
typedef uint16_t ui_recv_t;

/* User Interrupt Sender Table */
struct uist {
    uist_entry_t *data;
};

typedef struct uist ui_send_t;

#define UINTR_PTR(r) ((uintr_t *)(r))
#define UINTR_REF(p) ((word_t)(p))

#endif

typedef struct arch_tcb {
    user_context_t tcbContext;
#ifdef CONFIG_RISCV_UINTR
    uintr_t *tcbBoundUintr;
    uist_entry_t *tcbSenderTable;
#endif
} arch_tcb_t;

enum vm_rights {
    VMKernelOnly = 1,
    VMReadOnly = 2,
    VMReadWrite = 3
};
typedef word_t vm_rights_t;

typedef pte_t vspace_root_t;

/* Generic fastpath.c code expects pde_t for stored_hw_asid
 * that's a workaround in the time being.
 */
typedef pte_t pde_t;

#define PTE_PTR(r) ((pte_t *)(r))
#define PTE_REF(p) ((word_t)(p))

#define PT_SIZE_BITS 12
#define PT_PTR(r) ((pte_t *)(r))
#define PT_REF(p) ((word_t)(p))

#define PTE_SIZE_BITS   seL4_PageTableEntryBits
#define PT_INDEX_BITS   seL4_PageTableIndexBits

#define WORD_BITS   (8 * sizeof(word_t))
#define WORD_PTR(r) ((word_t *)(r))

static inline bool_t CONST cap_get_archCapIsPhysical(cap_t cap)
{
    cap_tag_t ctag;

    ctag = cap_get_capType(cap);

    switch (ctag) {

    case cap_frame_cap:
        return true;

    case cap_page_table_cap:
        return true;

    case cap_asid_control_cap:
        return false;

    case cap_asid_pool_cap:
        return true;

#ifdef CONFIG_RISCV_UINTR
    case cap_uintr_cap:
        return true;
#endif

    default:
        /* unreachable */
        return false;
    }
}

static inline word_t CONST cap_get_archCapSizeBits(cap_t cap)
{
    cap_tag_t ctag;

    ctag = cap_get_capType(cap);

    switch (ctag) {
    case cap_frame_cap:
        return pageBitsForSize(cap_frame_cap_get_capFSize(cap));

    case cap_page_table_cap:
        return PT_SIZE_BITS;

    case cap_asid_control_cap:
        return 0;

    case cap_asid_pool_cap:
        return seL4_ASIDPoolBits;

#ifdef CONFIG_RISCV_UINTR
    case cap_uintr_cap:
        return seL4_UintrBits;
#endif

    default:
        assert(!"Unknown cap type");
        /* Unreachable, but GCC can't figure that out */
        return 0;
    }
}

static inline void *CONST cap_get_archCapPtr(cap_t cap)
{
    cap_tag_t ctag;

    ctag = cap_get_capType(cap);

    switch (ctag) {

    case cap_frame_cap:
        return (void *)(cap_frame_cap_get_capFBasePtr(cap));

    case cap_page_table_cap:
        return PT_PTR(cap_page_table_cap_get_capPTBasePtr(cap));

    case cap_asid_control_cap:
        return NULL;

    case cap_asid_pool_cap:
        return ASID_POOL_PTR(cap_asid_pool_cap_get_capASIDPool(cap));

#ifdef CONFIG_RISCV_UINTR
    case cap_uintr_cap:
        return UINTR_PTR(cap_uintr_cap_get_capUintrPtr(cap));
#endif

    default:
        assert(!"Unknown cap type");
        /* Unreachable, but GCC can't figure that out */
        return NULL;
    }
}

static inline bool_t CONST Arch_isCapRevocable(cap_t derivedCap, cap_t srcCap)
{
#ifdef CONFIG_RISCV_UINTR
    if (cap_get_capType(derivedCap) == cap_uintr_cap) {
        return (cap_uintr_cap_get_capUintrBadge(derivedCap) !=
                cap_uintr_cap_get_capUintrBadge(srcCap));
    }
#endif
    return false;
}

#endif /* !__ASSEMBLER__  */

