#include "arch/i686/idt.h"

#include <arch/i686/io.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  uint16_t isr_low;   // The lower 16 bits of the ISR's address
  uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS
                      // before calling the ISR
  uint8_t reserved;   // Set to zero
  uint8_t attributes; // Type and attributes; see the IDT page
  uint16_t isr_high;  // The higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

__attribute__((aligned(0x10))) static idt_entry_t idt[256]; // IDT entries

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idtr_t;

static idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
  idt_entry_t *descriptor = &idt[vector];

  descriptor->isr_low = (uint32_t)isr & 0xFFFF;
  descriptor->kernel_cs = 0x08; // this value can be whatever offset your kernel
                                // code selector is in your GDT
  descriptor->attributes = flags;
  descriptor->isr_high = (uint32_t)isr >> 16;
  descriptor->reserved = 0;
}

#define IDT_MAX_DESCRIPTORS 256

extern void *isr_stub_table[]; // 0..31 exception stubs
extern void *irq_stub_table[]; // 0..15  IRQ stubs

static void pic_remap(uint8_t master_offset, uint8_t slave_offset) {
  i686_inb(0x21);
  i686_inb(0xA1);

  i686_outb(0x20, 0x11);
  i686_outb(0xA0, 0x11);          // start init
  i686_outb(0x21, master_offset); // remap master to 0x20
  i686_outb(0xA1, slave_offset);  // remap slave to 0x28
  i686_outb(0x21, 0x04);          // tell master: slave at IRQ2
  i686_outb(0xA1, 0x02);          // tell slave: cascade identity
  i686_outb(0x21, 0x01);
  i686_outb(0xA1, 0x01); // 8086/88 mode

  // restore masks (or set to 0xFF to mask all)
  i686_outb(0x21, 0xFF); // mask all master IRQs for now
  i686_outb(0xA1, 0xFF); // mask all slave IRQs
}

static inline void pic_eoi(int irq) {
  if (irq >= 8)
    i686_outb(0xA0, 0x20);
  i686_outb(0x20, 0x20);
}

void isr_c_handler(uint32_t int_no, uint32_t err_code) {
  terminal_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
  printf("EXC %u err=%08x\n", int_no, err_code);
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  for (;;)
    __asm__ volatile("cli; hlt");
}

void irq_c_handler(uint32_t irq) { pic_eoi(irq); }

void i686_init_idt(void) {
  idtr.base = (uintptr_t)&idt[0];
  idtr.limit = sizeof(idt) - 1;

  // exceptions 0..31
  for (uint8_t v = 0; v < 32; v++) {
    idt_set_descriptor(v, isr_stub_table[v],
                       0x8E); // present | DPL0 | 32-bit int gate
  }

  // IRQs 0..15 at 0x20..0x2F
  for (uint8_t i = 0; i < 16; i++) {
    idt_set_descriptor(0x20 + i, irq_stub_table[i], 0x8E);
  }

  __asm__ volatile("lidt %0" : : "m"(idtr));

  // Remap PIC to 0x20/0x28 and keep masked
  pic_remap(0x20, 0x28);
  i686_interrupts_enable();
}