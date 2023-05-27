#pragma once

#define UINTR_MAX_UIST_NR 256

void uintr_return(void);

void uintr_save(void);

int syscall_register_receiver(void);

int syscall_register_sender(void);