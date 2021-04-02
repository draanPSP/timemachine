.set noreorder

.section .text.vector
.global _start

.ent _start
_start:
# Clear bss
    la $a0, __bss_start
    la $a1, __bss_end
clear_bss_loop:
    sltu $a2, $a0, $a1
    addiu $a0, $a0, 4
    bnel $a2, $0, clear_bss_loop
    sw $0, -4($a0)
    
# Reset vector region is not big enough, place stack in VRAM instead
    la $sp, 0x04100000

#    jal _init
    move $gp, $0
    
    jal main
    nop

#    jal _fini
#    nop   
    
# We should never get here. However if we did for some reason,
# jump to bootrom's worker code in scratchpad. We skip some instructions
# from the start to force the MemoryStick path of execution.
    li $sp, 0x80013FF0
    li $t9, 0x80010070
    jr $t9
.end _start
.set reorder
