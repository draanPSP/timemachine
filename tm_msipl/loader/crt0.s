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
# we cannot jump to bootrom "as-is" like in the legacy case because 
# it uses 0xBFD00000 memory range - it is no longer available
# (it became 0xBFC00000). Patches to the bootrom have to be made.
# For now, just loop endlessly.
inf_loop:
    b inf_loop
    nop
.end _start
.set reorder
