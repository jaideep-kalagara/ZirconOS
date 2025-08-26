
; multiboot 2 header is handled in src/arch/i686/multiboot2_header.c

; stack
section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB is reserved for stack
stack_top:

; The linker script specifies _start as the entry point to the kernel
section .text
global _start:function (_start.end - _start)
_start:
    ; point to the top of the stack
	mov esp, stack_top


    ; kernel entry point
	extern kernel_main
	call kernel_main


	cli
.hang:	hlt
	jmp .hang
.end: