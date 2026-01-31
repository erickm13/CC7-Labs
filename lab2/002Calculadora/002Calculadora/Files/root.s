.section .text
.global _start

_start:
b reset

reset:
ldr sp, =0x00020000   @ Set up stack pointer (within RAM)
bl main               @ Call the main function
loop:
b loop                @ Infinite loop to prevent exit
