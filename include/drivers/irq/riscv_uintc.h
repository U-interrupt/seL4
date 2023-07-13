#pragma once

#include <config.h>
#include <types.h>
#include <plat/machine/devices_gen.h>
#include <arch/model/smp.h>

#define UINTC_WIDTH 32

#define UINTC_PPTR_BASE UINTC_PPTR

static inline uint64_t readq(word_t addr) {
    return *((volatile uint64_t *)(addr));
}

static inline void writeq(uint64_t val, word_t addr) {
    *((volatile uint64_t *)(addr)) = val;
}

static inline uint64_t uintc_read_low(int idx, uint64_t *val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x8;
    *val = readq(addr);
    return 0;
}

static inline uint64_t uintc_read_high(int idx, uint64_t *val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x10;
    *val = readq(addr);
    return 0;

}

static inline void uintc_write_low(int idx, uint64_t val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x8;
    writeq(val, addr);
}

static inline void uintc_write_high(int idx, uint64_t val) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH + 0x10;
    writeq(val, addr);
}

static inline void uintc_send(int idx) {
    word_t addr = UINTC_PPTR_BASE + idx * UINTC_WIDTH;
    writeq(0x1, addr);
}