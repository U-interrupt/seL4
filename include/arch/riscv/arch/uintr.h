#pragma once

#include <config.h>
#include <util.h>

#define UINTR_MAX_UIST_NR 4096

#ifndef __ASSEMBLER__

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

struct uintr_receiver {  
	uint16_t uirs_index;
	/* trace active vector per bit */
	uint64_t uvec_mask;
};
typedef struct uintr_receiver uintr_receiver_t;

/* User Interrupt Sender Status Table Context */
struct uist_ctx {
	struct uist_entry *uist;
	int uist_entry_idx;

    int refs;
};
typedef struct uist_ctx uist_ctx_t;

/* User Interrupt Sender */
struct uintr_sender {
	struct uist_ctx *uist_ctx;
	/* track active uist entries per bit */
	uint64_t uist_mask[4];  // total 256 bits
};
typedef struct uintr_sender uintr_sender_t;

static void uintr_return(void);

static int syscall_register_receiver(void);

static int syscall_register_sender(void);

void uintr_save(void)
VISIBLE;

#else
#endif