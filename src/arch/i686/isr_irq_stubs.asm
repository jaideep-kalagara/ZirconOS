; isr_irq_stubs.asm  (NASM, 32-bit)
; ------------------
; Provides:
;   - isr_stub_table[32]  : exception stubs (0..31)
;   - irq_stub_table[16]  : IRQ stubs (0..15, vectors 0x20..0x2F)
; Calls into C:
;   void isr_c_handler(uint32_t int_no, uint32_t err_code);
;   void irq_c_handler(uint32_t irq_no);

bits 32

extern isr_c_handler
extern irq_c_handler

; -------------------------
; Common prologue/epilogue
; -------------------------
; Stack on entry (normalized by macros below) will be:
;   TOP -> [int_no][err_code] ... CPU-pushed (EIP,CS,EFLAGS[,old SS/ESP])

isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10          ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; fetch args placed BEFORE pusha/push segs
    mov eax, [esp + 48]   ; int_no
    mov ebx, [esp + 52]   ; err_code

    ; cdecl: push right-to-left
    push ebx              ; err_code
    push eax              ; int_no
    call isr_c_handler
    add  esp, 8

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add  esp, 8           ; drop [int_no][err_code]
    iretd

irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; args are the same layout: [int_no][err_code(=0)]
    mov eax, [esp + 48]   ; int_no (this is 0x20..0x2F)
    sub eax, 0x20         ; convert to IRQ number 0..15

    push eax              ; irq_no
    call irq_c_handler
    add  esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add  esp, 8           ; drop [int_no][0]
    iretd


; -------------------------
; Macros for stubs
; -------------------------
; For exceptions WITHOUT CPU error code:
;   push 0 (fake err), push int_no -> TOP: [int_no][0]
%macro ISR_NOERR 1
isr_stub_%1:
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

; For exceptions WITH CPU error code:
; On entry stack already has [err_code]. We push int_no so TOP is [int_no][err_code].
%macro ISR_ERR 1
isr_stub_%1:
    push dword %1
    jmp isr_common_stub
%endmacro

; For IRQs (no CPU err code). Vector = 0x20+N, we’ll convert to IRQ in common.
%macro IRQ_STUB 2
irq_stub_%1:
    push dword 0x%2       ; int_no (vector)
    push dword 0          ; fake err_code
    jmp irq_common_stub
%endmacro


; -------------------------
; Exception stubs 0..31
; -------------------------
global isr_stub_table

; Exceptions that PUSH an error code by the CPU:
; 8, 10, 11, 12, 13, 14, 17, 30   (others are no-err)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

isr_stub_table:
%assign i 0
%rep 32
    dd isr_stub_%+i
%assign i i+1
%endrep


; -------------------------
; IRQ stubs 0..15 (vectors 0x20..0x2F)
; -------------------------
global irq_stub_table

IRQ_STUB 0, 20
IRQ_STUB 1, 21
IRQ_STUB 2, 22
IRQ_STUB 3, 23
IRQ_STUB 4, 24
IRQ_STUB 5, 25
IRQ_STUB 6, 26
IRQ_STUB 7, 27
IRQ_STUB 8, 28
IRQ_STUB 9, 29
IRQ_STUB 10, 2A
IRQ_STUB 11, 2B
IRQ_STUB 12, 2C
IRQ_STUB 13, 2D
IRQ_STUB 14, 2E
IRQ_STUB 15, 2F

irq_stub_table:
%assign j 0
%rep 16
    dd irq_stub_%+j
%assign j j+1
%endrep
