#include <assert.h>

#include <types.h>
#include <object/structures.h>
#include <object/tcb.h>

#include <arch/object/uintr.h>

#ifdef CONFIG_RISCV_UINTR

void bindUintr(tcb_t *tcb, uintr_t *uintrPtr)
{
    uintr_ptr_set_uintrBoundTCB(uintrPtr, (word_t)tcb);
    tcb->tcbArch.tcbBoundUintr = uintrPtr;
}

static inline void doUnbindUintr(uintr_t *uintrPtr, tcb_t *tcb)
{
    uintr_ptr_set_uintrBoundTCB(uintrPtr, (word_t)0);
    tcb->tcbArch.tcbBoundUintr = NULL;
}

void unbindUintr(tcb_t *tcb)
{
    uintr_t *uintrPtr;
    uintrPtr = tcb->tcbArch.tcbBoundUintr;
    if (uintrPtr) {
        doUnbindUintr(uintrPtr, tcb);
    }
}

void unbindMaybeUintr(uintr_t *uintrPtr)
{
    tcb_t *boundTCB;
    boundTCB = TCB_PTR(uintr_ptr_get_uintrBoundTCB(uintrPtr));

    if (boundTCB) {
        doUnbindUintr(uintrPtr, boundTCB);
    }
}

void receiveUintr(tcb_t *tcb, cap_t cap)
{
    uintr_t *uintrPtr;
    uint64_t pending;

    uintrPtr = UINTR_PTR(cap_uintr_cap_get_capUintrPtr(cap));

    /* read status from UINTC and clear pending bits */
    uintc_read_high(uintr_ptr_get_uintrIndex(uintrPtr), &pending);
    uintr_ptr_set_uintrPending(uintrPtr, pending);
    if (pending) {
        setRegister(tcb, badgeRegister, pending);
    } else {
        /* TODO: How to delegate user-interrupt back to supervisor ? */
        thread_state_ptr_set_tsType(&tcb->tcbState, ThreadState_BlockedOnUintr);
        thread_state_ptr_set_blockingObject(&tcb->tcbState, UINTR_REF(uintrPtr));
        scheduleTCB(tcb);
    }
}

void cancelUintr(uintr_t *uintrPtr)
{
    tcb_t *tcb;
    tcb = TCB_PTR(uintr_ptr_get_uintrBoundTCB(uintrPtr));
    if (tcb != NULL && thread_state_ptr_get_tsType(&tcb->tcbState) == ThreadState_BlockedOnUintr) {
        setThreadState(tcb, ThreadState_Restart);
        SCHED_ENQUEUE(tcb);
        rescheduleRequired();
    }
}

#define MAX_NUM 16
#define UIST_SIZE 512
#define UINTC_SIZE 512

bool_t uist_pool_used[MAX_NUM];
uist_entry_t uist_pool[MAX_NUM][UIST_SIZE] __attribute__((aligned(0x1000)));
bool_t uiste_used[MAX_NUM][UIST_SIZE];
bool_t uintc_pool_used[UINTC_SIZE];

word_t registerUintrReceiver(void)
{
    int i;
    bool_t full = true;

    for(i = 1; i < UINTC_SIZE; i ++) {
        if(!uintc_pool_used[i]) {
            uintc_pool_used[i] = true;
            full = false;
            break;
        }
    }
    if (full) return 0;

    printf("Alloc UINTC index=%d\n", i);

    return i;
}

int registerUintrSender(tcb_t *tcb, uintr_t *uintrPtr, cte_t *slot)
{
    uist_entry_t *entry;
    int pool_index = 0;

    /* allocate user-interrupt sender status table */
    if (!tcb->tcbArch.tcbSenderTable) {
        int i;
        bool_t full = true;
        for(i = 0; i < MAX_NUM; i++) {
            if(!uist_pool_used[i]) {
                uist_pool_used[i] = true;
                full = false;
                break;
            }
        }
        if (full) return -1;
        tcb->tcbArch.tcbSenderTable = &uist_pool[i][0];
        pool_index = i;
        printf("Alloc UIST index=%d base=%p\n", i, uist_pool[i]);
    }

    int i;
    bool_t full = true;
    for (i = 0; i < UIST_SIZE; i++) {
        if (!uiste_used[pool_index][i]) {
            uiste_used[pool_index][i] = true;
            full = false;
            break;
        }
    }
    if (full) return -1;

    entry = &uist_pool[pool_index][i];
    entry->send_vec = cap_uintr_cap_get_capUintrBadge(slot->cap);
    entry->uirs_index = uintr_ptr_get_uintrIndex(uintrPtr);
    entry->valid = 1;
    entry->reserved0 = entry->reserved1 = 0;

    printf("Alloc sender entry vec=%d index=%d uirs_index=%d this_uirs_index=%llu\n", entry->send_vec, i, entry->uirs_index,  uintr_ptr_get_uintrIndex(tcb->tcbArch.tcbBoundUintr));

    slot->cap = cap_uintr_cap_set_capUintrSendIndex(slot->cap, i);
    slot->cap = cap_uintr_cap_set_capUintrSendBase(slot->cap, (word_t)uist_pool[pool_index]);

    return i;
}

#endif /* CONFIG_RISCV_UINTR */
