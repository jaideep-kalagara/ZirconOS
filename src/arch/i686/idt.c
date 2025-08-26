#include "arch/i686/idt.h"
#include <util/binary.h>

typedef struct {
  uint16_t base_low;
  uint16_t segment_selector;
  uint8_t reserved;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
  uint16_t limit;
  idt_entry_t *ptr;
} __attribute__((packed)) idt_descriptor_t;

idt_entry_t IDT[256];

idt_descriptor_t IDTR = {.limit = sizeof(IDT) - 1, .ptr = IDT};

extern void i686_idt_load(idt_descriptor_t *idt_descriptor);

void i686_idt_set_gate(int interrupt, void *base, uint16_t segment_descriptor,
                       uint8_t flags) {
  IDT[interrupt].base_low = ((uint32_t)base) & 0xFFFF;
  IDT[interrupt].segment_selector = segment_descriptor;
  IDT[interrupt].reserved = 0;
  IDT[interrupt].flags = flags;
  IDT[interrupt].base_high = ((uint32_t)base >> 16) & 0xFFFF;
}

void i686_idt_enable_gate(int interrupt) {
  FLAG_SET(IDT[interrupt].flags, IDT_FLAG_PRESENT);
}
void i686_idt_disable_gate(int interrupt) {
  FLAG_UNSET(IDT[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_init_idt(void) { i686_idt_load(&IDTR); }