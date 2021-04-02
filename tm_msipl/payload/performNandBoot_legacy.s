.global performNandBoot

# Syscon triggers service mode in the bootrom code via TACHYON_SPI_CS GPIO pin high
# We have initialized syscon in the payload - as a result, the pin is currently low
# To boot from NAND, we can just jump to bootrom's worker code in the scratchpad - in
# the current situation it will act as the service mode was never triggered

performNandBoot:
    li $sp, 0x80013FF0
    li $t9, 0x80010000
    jr $t9
    nop
