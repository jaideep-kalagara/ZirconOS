#include "kernel/sleep.h"

#include <arch/i686/irq.h>
#include <stdio.h>

uint32_t countdown = 0;

void sleep(uint32_t ms) {
    countdown = (ms * 100 + 999) / 1000;
    while (countdown > 0) {
        __asm__ volatile("hlt");
    }
}

void time_tick(registers *regs) {
    if (countdown > 0) {
        countdown--;
    }
}

void init_sleep() { i686_irq_register_handler(0, time_tick); }