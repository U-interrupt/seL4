#include <arch/uintr.h>
#include <drivers/irq/riscv_uintc.h>
#include <api/syscall.h>

#define MAX_SIZE 16
#define UINTC_MAX_NR 512
#define ENOMEM 12

uist_ctx_t uist_ctx_pool[MAX_SIZE];
uist_entry_t uist_entry_pool[MAX_SIZE][UINTR_MAX_UIST_NR] __attribute__((aligned(0x1000)));
uintr_sender_t uintr_sender_pool[MAX_SIZE];
uintr_receiver_t uintr_recv_pool[MAX_SIZE];
char uist_ctx_used[MAX_SIZE];
char uist_entry_used[MAX_SIZE];
char uintr_sender_used[MAX_SIZE];
char uintc_used[UINTC_MAX_NR];
char uintr_recv_used[MAX_SIZE];
word_t buffer_for_args[32];

static void free_uist(uist_ctx_t *uist_ctx)
{
	//free_kernel_object(uist_ctx -> uist);
	uist_ctx_used[uist_ctx -> uist_entry_idx] = 0;
	uist_ctx -> uist = NULL;

	//free_kernel_object(uist_ctx);
	uist_ctx_used[(int)(uist_ctx - uist_ctx_pool)] = 0;
}

static uist_ctx_t *alloc_uist(void)
{
	uist_ctx_t *uist_ctx = NULL;
	uist_entry_t *uist = NULL;

	//uist_ctx = alloc_kernel_object(sizeof(*uist_ctx));
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uist_ctx_used[i]) {
			uist_ctx_used[i] = 1;
			uist_ctx = &uist_ctx_pool[i];
			uist_ctx -> uist_entry_idx = i;
			break;
		}
	}
	if (! uist_ctx)
		return NULL;

	//uist = alloc_kernel_object(sizeof(*uist) * UINTR_MAX_UIST_NR);
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uist_entry_used[i]) {
			uist_entry_used[i] = 1;
			uist = uist_entry_pool[i];
			printf("Alloc Uist entry: i = %d, %lx\n", i, (unsigned long) uist);
			break;
		}
	}
	if (!uist) {
		//free_kernel_object(uist_ctx);
		uist_ctx_used[(int)(uist_ctx - uist_ctx_pool)] = 0;
		return NULL;
	}

	uist_ctx -> uist = uist;
	uist_ctx -> refs = 1;

	return uist_ctx;
}

[[maybe_unused]] static void put_uist_ref(struct uist_ctx *uist_ctx)
{
	if ( (-- uist_ctx -> refs) == 0)
		free_uist(uist_ctx);
}

[[maybe_unused]] static struct uist_ctx *get_uist_ref(struct uist_ctx *uist_ctx)
{
	++ uist_ctx -> refs;
	return uist_ctx;
}

static int init_sender(void)
{
	printf("init_sender\n");
	tcb_t *tcb = NODE_STATE(ksCurThread);
	struct uintr_sender *ui_send = NULL;

	//ui_send = alloc_kernel_object(sizeof(*ui_send));
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uintr_sender_used[i]) {
			uintr_sender_used[i] = true;
			ui_send = &uintr_sender_pool[i];
			break;
		}
	}
	
	if (!ui_send)
		return -ENOMEM;

	ui_send->uist_ctx = alloc_uist();
	if (!ui_send->uist_ctx) {
		puts("Failed to allocate user-interrupt sender table\n");
		//free_kernel_object(ui_send);
		uintr_sender_used[(int)(ui_send - uintr_sender_pool)] = false;

		return -ENOMEM;
	}

	tcb -> ui_send = ui_send;
	return 0;
}

static void load_uirs(unsigned int entry, struct uirs_entry *uirs)
{
	unsigned long long low, high;

	uintc_read_low(entry, &low);
	uintc_read_high(entry, &high);

	uirs->mode = low;
	uirs->hartid = low >> 16;
	uirs->irq = high;

	/* reserved bits ignored */
	uirs->reserved0 = uirs->reserved1 = 0;
	// printf("loaded uirs(%lx): entry = %d, mode = %d\n", (unsigned long) uirs, entry, uirs -> mode);
}

static void store_uirs(unsigned entry, struct uirs_entry *uirs)
{
	unsigned long long low, high;

	low = uirs->mode | (uirs->hartid << 16);
	high = uirs->irq;

	uintc_write_low(entry, low);
	uintc_write_high(entry, high);
	// printf("stored uirs(%lx): entry = %d, mode = %d\n", (unsigned long) uirs, entry, uirs -> mode);
}

static int syscall_register_receiver(void) {
	tcb_t *tcb = NODE_STATE(ksCurThread);
	if(tcb -> ui_recv != NULL) {
		puts("Already registered a user-interrupt receiver");
		return -1;
	}
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uintr_recv_used[i]) {
			uintr_recv_used[i] = true;
			tcb -> ui_recv = &uintr_recv_pool[i];
			break;
		}
	}
	if(! tcb -> ui_recv) {
		puts("Failed to allocate user-interrupt sender table\n");
		return -ENOMEM;
	}
	int uintc_idx = -1;
	for(int i = 0; i < UINTC_MAX_NR; i ++) {
		if(! uintc_used[i]) {
			uintc_used[i] = true;
			uintc_idx = i;
			break;
		}
	}
	if(uintc_idx == -1) {
		puts("Failed to allocate user-interrupt sender table\n");
		return -ENOMEM;
	}
	printf("Syscall register receiver: uintc_idx = %d\n", uintc_idx);
	uintc_write_low(uintc_idx, 0LL);
	uintc_write_high(uintc_idx, 0LL);
	tcb -> ui_recv -> uirs_index = uintc_idx;
	tcb -> utvec = csr_read(CSR_UTVEC) & 0xFFFFFFFFFFFFFFF8; //direct mode
	tcb -> uscratch = csr_read(CSR_USCRATCH);
	puts("Syscall register receiver Done!");
	setRegister(tcb, a0, uintc_idx); // return uintc_idx
	return 0;
}

static int syscall_register_sender(void) {
	word_t receiver_idx = getRegister(NODE_STATE(ksCurThread), a0);
	word_t vec = getRegister(NODE_STATE(ksCurThread), a1);
	tcb_t *tcb = NODE_STATE(ksCurThread);
	int sender_idx = -1;
	if(tcb -> ui_send == NULL) {
		//Init sender
		init_sender();
	}
	for(int i = 0; i < UINTR_MAX_UIST_NR; i ++) {
		int idx = i / 64;
		int offset = i % 64;
		if(! (tcb -> ui_send -> uist_mask[idx] & (1LL << offset))) {
			sender_idx = i;
			tcb -> ui_send -> uist_mask[idx] |= (1LL << offset);
			break;
		}
	}
	if(sender_idx == -1) {
		puts("Failed to allocate user-interrupt sender table\n");
		return -ENOMEM;
	}
	tcb -> ui_send -> uist_ctx -> uist[sender_idx].send_vec = vec;
	tcb -> ui_send -> uist_ctx -> uist[sender_idx].uirs_index = receiver_idx;
	tcb -> ui_send -> uist_ctx -> uist[sender_idx].valid = 0x1;
	printf("Syscall register sender: receiver_idx = %ld, vec = %ld,  sender_idx = %d\n", receiver_idx, vec, sender_idx);
	setRegister(tcb, a0, sender_idx); // return sender_idx 
	return 0;
} 

static void uirs_restore(void) {
	csr_set(CSR_SIDELEG, IE_USIE);

	tcb_t *tcb = NODE_STATE(ksCurThread);
	if(tcb -> ui_recv == NULL) {
		csr_write(CSR_SUIRS, 0UL);
		csr_clear(CSR_UIE, IE_USIE);
		csr_clear(CSR_UIP, IE_USIE);
		return;
	}

	uirs_entry_t uirs;
	load_uirs(tcb -> ui_recv -> uirs_index, &uirs);
	uirs.hartid = 0; //getCurrentCPUIndex(); #TODO: SMP support
	uirs.mode |= 0x2;
	store_uirs(tcb -> ui_recv -> uirs_index, &uirs);
	csr_write(CSR_SUIRS, (1UL << 63) | tcb -> ui_recv -> uirs_index);

	csr_set(CSR_UIE, IE_USIE);
	if (uirs.irq)
		csr_set(CSR_UIP, IE_USIE);
	else
		csr_clear(CSR_UIP, IE_USIE);
	
	// User interrupt registers restore:
	csr_write(CSR_UTVEC, tcb -> utvec);
	csr_write(CSR_USCRATCH, tcb -> uscratch);
	csr_write(CSR_UEPC, tcb -> uepc);
	// puts("UIRS RESTORE DONE!");
}

static void uist_init(void) {
	tcb_t *tcb = NODE_STATE(ksCurThread);
	if(tcb -> ui_send == NULL) {
		csr_write(CSR_SUIST, 0UL);
		return;
	}
	// printf("UIST INIT: uist = %lx\n", (unsigned long) tcb -> ui_send -> uist_ctx -> uist);
	csr_write(CSR_SUIST, (1UL << 63) | (1UL << 44) | (kpptr_to_paddr(tcb -> ui_send -> uist_ctx -> uist) >> 0xC));
}

/// Called during trap return.
static void uintr_return(void) {
	csr_write(CSR_SUICFG, 0x2f10000);
	//receiver
	uirs_restore();
	//sender
	uist_init();
}

/// Called when task traps into kernel.
[[maybe_unused]] void uintr_save(void) {
	tcb_t *tcb = NODE_STATE(ksCurThread);
	tcb -> uepc = csr_read(CSR_UEPC);
}
