.syntax unified
.cpu arm7tdmi
.section .header,"ax"
.arm
.global _start
_start:
    b boot
    .space 188
boot:
    @ No interrupts: VBlank is polled. Use the system-mode stack.
    mov r0, #0xdf
    msr cpsr_c, r0
    ldr sp, =0x03007f00
    ldr r0, =0x04000208
    mov r1, #0
    strh r1, [r0]
    ldr r0, =0x04000204
    ldr r1, =0x4317
    strh r1, [r0]
    ldr r0, =__text_load
    ldr r1, =__text_start
    ldr r2, =__text_end
1:  cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 1b
    ldr r0, =__data_load
    ldr r1, =__data_start
    ldr r2, =__data_end
2:  cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 2b
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
3:  cmp r1, r2
    strlo r3, [r1], #4
    blo 3b
    ldr r0, =main
    bx r0
    .pool
