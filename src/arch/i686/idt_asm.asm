[bits 32]

; void i686_idt_load(idt_descriptor_t* idt_descriptor);
global i686_idt_load
i686_idt_load:

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame
    
    ; load idt
    mov eax, [ebp + 8]
    lidt [eax]

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret