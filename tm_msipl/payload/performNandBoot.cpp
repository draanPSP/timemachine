#include "performBoot.h"

void performNandBoot() {
    // Firmware IPL operates on 0xBFD00000 memory range and ROM mapped to 0xBFC00000,
    // both are no longer accessible. This dummy infinite loop is put there until
    // patches to the IPL are developed to handle this case.
    while(true) {
        asm("\n");
    }
}