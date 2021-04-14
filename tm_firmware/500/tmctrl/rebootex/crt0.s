.set noreorder

.section .text.vector
.global _start

.ent _start
_start:
    j main
    nop
.end _start
.set reorder
