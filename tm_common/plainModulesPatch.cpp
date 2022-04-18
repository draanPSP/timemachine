/* 
 * Based on plain2x/loadcore.c from Devhook - https://github.com/mathieulh/Devhook
*/

#include <plainModulesPatch.h>

#define ELF_MAGIC 0x464c457f

int (* _sceKernelCheckExecFile)(u8 *buf, LoadCoreExecInfo *execInfo);

static u16 setModAttr(u8 *buf, LoadCoreExecInfo *execInfo)
{
    SceModuleInfo *modinfo = (SceModuleInfo *)&(buf[execInfo->modInfoOffset]);
    u16 attr = modinfo->modattribute;
    execInfo->modAttr = attr; 

    return attr;
}

int sceKernelCheckExecFile_before(u8 *buf, LoadCoreExecInfo *execInfo)
{
	if(((u32 *)buf)[0] == ELF_MAGIC)
	{
		int app_type  = execInfo->apiType;

		switch(app_type)
		{
			case 0x120: // UMD boot module
			case 0x141: // MS GAME module
				if(execInfo->size)
				{
					// escape ProbeExec error at 2nd call
					execInfo->isKernel = 1;
					execInfo->isDecrypted = 1;

					setModAttr(buf,execInfo);
					return 0;
				}

			case 0x050: // kernel module : init
			case 0x051: // kernel module : many
				if(execInfo->isKernel)
				{
					// need decrypt after camouflage ELF
					// set decrypted flag
					execInfo->isDecrypted = 1;
					return 0;
				}
				break;
		}
	}

	return -1; // no hook return
}

int sceKernelCheckExecFile_after(u8 *buf, LoadCoreExecInfo *execInfo, int isELF, int result)
{
	// camouflage plain ELF
	if(isELF)
	{
		int app_type  = execInfo->apiType;

		switch(app_type)
		{

			case 0x050: // kernel module : init
			case 0x051: // kernel module : many
				if(setModAttr(buf, execInfo) & 0xff00)
				{
					// set crypt flag if kernel/vsh module
					execInfo->isKernel = 1;
					result = 0;
				}
				break;
			}
	}

	return result;
}

int sceKernelCheckExecFileHook(u8 *buf, LoadCoreExecInfo *execInfo)
{
    int result;
    int isELF;

    if( sceKernelCheckExecFile_before(buf, execInfo) == 0)
        return 0;

    isELF = (((u32 *)buf)[0] == ELF_MAGIC);

    result = _sceKernelCheckExecFile(buf, execInfo);

    return sceKernelCheckExecFile_after(buf, execInfo, isELF, result);
}