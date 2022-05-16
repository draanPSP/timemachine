.set noreorder

.section .text.vector
.global _start

.ent _start
_start:
# Clear bss
    la $t0, __bss_start
    la $t1, __bss_end
clear_bss_loop:
    sltu $t2, $t0, $t1
    addiu $t0, $t0, 4
    bnel $t2, $0, clear_bss_loop
    sw $0, -4($t0)
    j main
    nop
.end _start
.set reorder
