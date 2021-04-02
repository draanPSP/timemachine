#ifndef PAYLOAD_PERFORMBOOT
#define PAYLOAD_PERFORMBOOT

#include <ff.h>

extern "C" {
    //This function is provided either by the assembly file (in legacy mode) or by the c++ file
    [[noreturn]] void performNandBoot();
    
    //This function is provided by different c++ files, depending on mode
    [[noreturn]] void performMsBoot(FIL &fp);
}

#endif //PAYLOAD_PERFORMNANDBOOT