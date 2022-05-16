#ifndef __TM_PLAIN_MODULES
#define __TM_PLAIN_MODULES

#ifdef __cplusplus
extern "C" {
#endif

#include <pspkerneltypes.h>
#include <pspmoduleinfo.h>

#define SCE_KERNEL_MAX_MODULE_SEGMENT   (4) 

typedef struct {
    u32 unk0[2];
    u32 apiType;
    u32 unk1;
    u32 size;
    u32 unk2[12];
    u32 isKernel;
    u32 isDecrypted;
    u32 modInfoOffset;
    u32 unk3[2];
    u16 modAttr;
    u16 unk4;
    u32 unk5[2];
    u32 isSignChecked;
} __attribute__((__packed__)) LoadCoreExecInfo;

extern int (* _sceKernelCheckExecFile)(u8 *buf, LoadCoreExecInfo *execInfo);
int sceKernelCheckExecFileHook(u8 *buf, LoadCoreExecInfo *execInfo);

#ifdef __cplusplus
}
#endif

#endif
