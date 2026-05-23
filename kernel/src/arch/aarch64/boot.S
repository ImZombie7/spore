.section .text.boot, "ax"
.global _start
.type _start, %function
_start:
    bl kernel_main

1:
    wfe
    b 1b

