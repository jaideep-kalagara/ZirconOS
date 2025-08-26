#include "arch/i686/isr.h"
#include "arch/i686/idt.h"
#include "arch/i686/io.h"
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdio.h>

isr_handler_t isr_handlers[256];

static const char *const exceptions[] = {"Divide by zero error",
                                         "Debug",
                                         "Non-maskable Interrupt",
                                         "Breakpoint",
                                         "Overflow",
                                         "Bound Range Exceeded",
                                         "Invalid Opcode",
                                         "Device Not Available",
                                         "Double Fault",
                                         "Coprocessor Segment Overrun",
                                         "Invalid TSS",
                                         "Segment Not Present",
                                         "Stack-Segment Fault",
                                         "General Protection Fault",
                                         "Page Fault",
                                         "",
                                         "x87 Floating-Point Exception",
                                         "Alignment Check",
                                         "Machine Check",
                                         "SIMD Floating-Point Exception",
                                         "Virtualization Exception",
                                         "Control Protection Exception ",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "Hypervisor Injection Exception",
                                         "VMM Communication Exception",
                                         "Security Exception",
                                         ""};

void i686_isr_initialize_gates();

void i686_init_isr() {
  i686_isr_initialize_gates();
  for (int i = 0; i < 256; i++)
    i686_idt_enable_gate(i);

  i686_idt_disable_gate(0x80); // NMI
}

void isr_c_handler(registers *regs) {
  if (isr_handlers[regs->interrupt] != NULL)
    isr_handlers[regs->interrupt](regs);
  else if (regs->interrupt >= 32) {
    terminal_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    printf("Unhandled interrupt %d!\n", regs->interrupt);
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  } else {
    terminal_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    printf("Unhandled exception %d %s\n", regs->interrupt,
           exceptions[regs->interrupt]);

    printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n", regs->eax,
           regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

    printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
           regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds,
           regs->ss);

    printf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);
    printf("KERNEL PANIC!");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    i686_panic();
  }
}

void i686_isr_register_handler(int interrupt, isr_handler_t handler) {
  isr_handlers[interrupt] = handler;
  i686_idt_enable_gate(interrupt);
}