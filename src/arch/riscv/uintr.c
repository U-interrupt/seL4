#include <arch/uintr.h>

#define MAX_NUM 16
#define UIST_SIZE 512
#define UINTC_SIZE 512

#ifdef CONFIG_RISCV_UINTR

bool_t uist_pool_used[MAX_NUM];
uist_entry_t uist_pool[MAX_NUM][UIST_SIZE] __attribute__((aligned(0x1000)));
bool_t uintc_pool_used[UINTC_SIZE];

[[maybe_unused]] static void alloc_uiste(tcb_t *tcb, word_t vector, word_t uintc_index, word_t index)
{
    uist_entry_t *entry;

    if (!tcb || index >= +UIST_SIZE) return;

    /* alloc uist */
    if (!tcb->tcbArch.tcbUISend.data) {
        int i;
        bool_t full = true;
        for(i = 0; i < MAX_NUM; i ++) {
            if(!uist_pool_used[i]) {
                uist_pool_used[i] = true;
                full = false;
                break;
            }
        }
        if (full) return;
        tcb->tcbArch.tcbUISend.data = &uist_pool[i][0];
        printf("Pool index=%d\n", i);
    }

    entry = &tcb->tcbArch.tcbUISend.data[index];
    entry->send_vec = vector;
    entry->uirs_index = uintc_index;
    entry->valid = 1;
    entry->reserved0 = entry->reserved1 = 0;

    printf("%lu alloc_uiste vec=%lu uirs_index=%lu index=%lu base=%lx\n",cpuIndexToID(getCurrentCPUIndex()), vector, uintc_index, index, (unsigned long)tcb->tcbArch.tcbUISend.data);
    
    return;
}

[[maybe_unused]] static void alloc_uintc(tcb_t *tcb)
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
    if (full) return;

    tcb->tcbArch.tcbUIRecv = i;
    printf("%lu alloc_uintc index=%d\n",cpuIndexToID(getCurrentCPUIndex()), i);

    uirs_entry_t entry;
    load_uirs(i, &entry);
    store_uirs(i, &entry);
}

#endif
