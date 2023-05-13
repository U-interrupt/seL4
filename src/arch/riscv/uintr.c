#include <arch/uintr.h>

static void free_kernel_object(void * ptr) {
    // TODO
}

static void* alloc_kernel_object(unsigned int size) {
    // TODO
}

static void free_uist(struct uist_ctx *uist_ctx)
{
	unsigned long flags;

	free_kernel_object(uist_ctx->uist);
	uist_ctx->uist = NULL;

	free_kernel_object(uist_ctx);
}

static struct uist_ctx *alloc_uist(void)
{
	struct uist_ctx *uist_ctx;
	struct uist_entry *uist;

	uist_ctx = alloc_kernel_object(sizeof(*uist_ctx));
	if (!uist_ctx)
		return NULL;

	uist = alloc_kernel_object(sizeof(*uist) * UINTR_MAX_UIST_NR);
	if (!uist) {
		free_kernel_object(uist_ctx);
		return NULL;
	}

	uist_ctx->uist = uist;
	spin_lock_init(&uist_ctx->uist_lock);
	refcount_set(&uist_ctx->refs, 1);

	return uist_ctx;
}

static void put_uist_ref(struct uist_ctx *uist_ctx)
{
	if (refcount_dec_and_test(&uist_ctx->refs))
		free_uist(uist_ctx);
}

static struct uist_ctx *get_uist_ref(struct uist_ctx *uist_ctx)
{
	refcount_inc(&uist_ctx->refs);
	return uist_ctx;
}

static int init_sender(void)
{
	struct task_struct *t = current;
	struct uintr_sender *ui_send;

	ui_send = alloc_kernel_object(sizeof(*ui_send));
	if (!ui_send)
		return -ENOMEM;

	ui_send->uist_ctx = alloc_uist();
	if (!ui_send->uist_ctx) {
		pr_debug("Failed to allocate user-interrupt sender table\n");
		free_kernel_object(ui_send);
		return -ENOMEM;
	}

	return 0;
}
