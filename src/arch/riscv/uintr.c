#include <arch/uintr.h>

const int MAX_SIZE = 16;

uist_ctx uist_ctx_pool[MAX_SIZE];
uist_entry uist_entry_pool[MAX_SIZE][UINTR_MAX_UIST_NR];
uintr_sender uintr_sender_pool[MAX_SIZE];
bool uist_ctx_used[MAX_SIZE];
bool uist_entry_used[MAX_SIZE];
bool uintr_sender_used[MAX_SIZE];

static void free_uist(struct uist_ctx *uist_ctx)
{
	unsigned long flags;

	//free_kernel_object(uist_ctx -> uist);
	uist_used[(int) ((uist_ctx -> uist) - uist_pool)] = false;
	uist_ctx -> uist = NULL;

	//free_kernel_object(uist_ctx);
	uist_ctx_used[(int)(uist_ctx - uist_ctx_pool)] = false;
}

static struct uist_ctx *alloc_uist(void)
{
	struct uist_ctx *uist_ctx = NULL;
	struct uist_entry *uist = NULL;

	//uist_ctx = alloc_kernel_object(sizeof(*uist_ctx));
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uist_ctx_used[i]) {
			uist_ctx_used[i] = true;
			uist_ctx = &uist_ctx_pool[i];
			break;
		}
	}
	if (! uist_ctx)
		return NULL;

	//uist = alloc_kernel_object(sizeof(*uist) * UINTR_MAX_UIST_NR);
	for(int i = 0; i < MAX_SIZE; i ++) {
		if(! uist_entry_used[i]) {
			uist_entry_used[i] = true;
			uist = uist_entry_pool[i];
		}
	}
	if (!uist) {
		//free_kernel_object(uist_ctx);
		uist_ctx_used[(int)(uist_ctx - uist_ctx_pool)] = false;
		return NULL;
	}

	uist_ctx -> uist = uist;
	uist_ctx -> refs = 1;

	return uist_ctx;
}

static void put_uist_ref(struct uist_ctx *uist_ctx)
{
	if ( (-- uist_ctx -> ref) == 0)
		free_uist(uist_ctx);
}

static struct uist_ctx *get_uist_ref(struct uist_ctx *uist_ctx)
{
	++ uist_ctx -> ref;
	return uist_ctx;
}

static int init_sender(void)
{
	// struct task_struct *t = current;
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
		uintr_sender_used[(int)(ui_send - uintr_sender)] = false;

		return -ENOMEM;
	}

	return 0;
}
