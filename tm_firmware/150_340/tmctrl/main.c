#include <string.h>

#include <pspidstorage.h>
#include <pspkernel.h>
#include <pspreg.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include <pspmacro.h>
#include <moduleUtils.h>

#include <rebootex.h>

PSP_MODULE_INFO("TimeMachine_Control", 0x2000 | PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_START | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_NO_STOP, 1, 0);

int (* doLinkLibraryEntries)(SceLibraryEntryTable *entryTable, u32 size, int isUserMode);

int (* GetMsSize)(void);

int start_thread;
static SceModule2 *last_module;
int last_thid = -1;

typedef int (* STMOD_HANDLER)(SceModule2 *);
STMOD_HANDLER stmod_handler = NULL;

int ValidateSeekPatched(u32 *drv_str, SceOff ofs)
{
	if ((ofs & 0x1FF))
		return 0;

	SceOff max_size = GetMsSize(); // size of partition in sectors
	max_size *= 512; // size in bytes

	if (ofs >= max_size)
		return 0;

	return 1;
}

int ValidateSeekP1Patched(u32 *drv_str, SceOff ofs)
{
	if ((ofs & 0x1FF))
		return 0;

	u32 *p = (u32 *)drv_str[0x10/4];
	u32 *q = (u32 *)p[8/4];
	SceOff max_size = (SceOff)q[4/4]; // partition_start

	max_size = GetMsSize() - max_size; // size of partition in sectors
	max_size *= 512; // size in bytes

	if (ofs >= max_size)
		return 0;

	return 1;
}

STMOD_HANDLER SetModuleStartUpHandler(STMOD_HANDLER handler)
{
	unsigned int k1 = pspSdkSetK1(0);

	STMOD_HANDLER res = stmod_handler;

	stmod_handler = (STMOD_HANDLER)(0x80000000 | (u32)handler);

	pspSdkSetK1(k1);
	return res;
}

int sceKernelStartThreadPatched(SceUID thid, SceSize arglen, void *argp)
{
	if (thid == start_thread)
	{
		start_thread = -1;

		if (last_module && stmod_handler)
			stmod_handler(last_module);
	}

	return sceKernelStartThread(thid, arglen, argp);
}

int sceKernelCreateThreadPatched(char *name, void *entry, int priority, int stackSize, int attr, SceKernelThreadOptParam *param)
{
	int thid = sceKernelCreateThread(name, entry, priority, stackSize, attr, param);
	if (thid >= 0 && strncmp(name, "SceModmgrStart", 14) == 0)
	{
		start_thread = thid;
		last_module = (SceModule2 *)sceKernelFindModuleByAddress((u32)entry);
	}

	return thid;
}

REGHANDLE lang_hk = -1;

int sceRegGetKeyValuePatched(REGHANDLE hd, REGHANDLE hk, void *buffer, SceSize size)
{
	int res = sceRegGetKeyValue(hd, hk, buffer, size);
	if (res >= 0 && hk == lang_hk)
	{
		if (*(u32 *)buffer > 8)
			*(u32 *)buffer = 1;
		
		lang_hk = -1;
	}
	return res;
}

int sceRegGetKeyInfoPatched(REGHANDLE hd, const char *name, REGHANDLE *hk, unsigned int *type, SceSize *size)
{
	int res = sceRegGetKeyInfo(hd, name, hk, type, size);

	if (res >= 0 && strcmp(name, "language") == 0)
	{
		if (hk)
			lang_hk = *hk;
	}

	return res;
}

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	sceKernelGzipDecompress((u8 *)0x88FC0000, 0x10000, rebootex, 0);

	return sceKernelGzipDecompress(dest, destSize, src, unk);
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

int doLinkLibraryEntriesPatched(SceLibraryEntryTable *entryTable, u32 size, int isUserMode)
{
	void *curEntryTable;
	void *entryTableMemRange;

	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByAddress(entryTable);

	char *moduleName = mod->modname;
	if ((strcmp(moduleName,"sceI2C_Driver") == 0 || 
		strcmp(moduleName,"sceIdStorage_Service") == 0 ||
		strcmp(moduleName,"sceWM8750_Driver") == 0)
		&& size != 0) {

		entryTableMemRange = (u32)entryTable + size;

		for (curEntryTable = entryTable; curEntryTable < entryTableMemRange; 
			 curEntryTable += ((SceLibraryEntryTable *)curEntryTable)->len * sizeof(u32)) {

			if (strcmp(((SceLibraryEntryTable *)curEntryTable)->libname, "ThreadManForKernel") != 0)
				continue;

			strcpy(((SceLibraryEntryTable *)curEntryTable)->libname, "ZhreadManForKernel");
			ClearCaches();
		}
	}

	return doLinkLibraryEntries(entryTable, size, isUserMode);
}

void PatchSceModuleManager()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceModuleManager");
	_sw(0x1000002A, mod->text_addr + 0x3F28);
	MAKE_JUMP(mod->text_addr + 0x4714, sceKernelCreateThreadPatched);
	MAKE_JUMP(mod->text_addr + 0x4724, sceKernelStartThreadPatched);
}

int OnModuleStart(SceModule2 *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName,"sceLoadExec") == 0) {
		MAKE_CALL(mod->text_addr + 0x2344, RebootBinDecompressPatched);
		_sw(0x3C0188FC, mod->text_addr + 0x2384); //LUI $at 0x88FC - Patch call to reboot.bin
		ClearCaches();
	}
	else if (strcmp(moduleName, "scePower_Service") == 0)
	{
		_sw(0, text_addr + 0xb24);
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceDisplay_Service") == 0)
	{
		MAKE_FUNCTION_RETURN0(mod->text_addr);
		ClearCaches();
	}
	if (strcmp(moduleName, "sceRegistry_Service") == 0)
	{
		_sw((u32)sceRegGetKeyInfoPatched, text_addr+0x76DC);
		_sw((u32)sceRegGetKeyValuePatched, text_addr+0x76E0);

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceLflashFatfmt") == 0)
	{
		u32 funcAddr = GetModuleExportFuncAddr("sceLflashFatfmt", "LflashFatfmt", 0xB7A424A4); //sceLflashFatfmtStartFatfmt
		if (funcAddr)
		{
			MAKE_FUNCTION_RETURN0(funcAddr);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "sceMSstor_Driver") == 0)
	{
		REDIRECT_FUNCTION(text_addr + 0x5138, ValidateSeekPatched);
		REDIRECT_FUNCTION(text_addr + 0x51bc, ValidateSeekP1Patched);
		GetMsSize = (void *)(text_addr + 0x0288);
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceWlan_Driver") == 0)
	{
		_sw(0, mod->text_addr + 0x2520);
		ClearCaches();
	}

	return 0;
}

int sceKernelDeleteFplPatched(SceUID uid)
{
	return sceKernelDeleteFpl(uid);
}

int sceKernelAllocateFplPatched(SceUID uid, void **data, unsigned int *timeout)
{
	return sceKernelAllocateFpl(uid, data, timeout);
}

int sceKernelCreateFplPatched(const char *name, int part, int attr, unsigned int size, unsigned int blocks, struct SceKernelFplOptParam *opt)
{
	return sceKernelCreateFpl(name, part, attr, size, blocks, opt);
}

int sceKernelDeleteEventFlagPatched(int evid)
{
	return sceKernelDeleteEventFlag(evid);
}

int sceKernelWaitEventFlagPatched(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout)
{
	return sceKernelWaitEventFlag(evid, bits, wait, outBits, timeout);
}

int sceKernelSetEventFlagPatched(SceUID evid, u32 bits)
{
	return sceKernelSetEventFlag(evid, bits);
}

SceUID sceKernelCreateEventFlagPatched(const char *name, int attr, int bits, SceKernelEventFlagOptParam *opt)
{
	return sceKernelCreateEventFlag(name, attr, bits, opt);
}

int sceKernelDelayThreadPatched(SceUInt delay)
{
	return sceKernelDelayThread(delay);
}	

int sceKernelDeleteMutexPatched(SceUID	mutexid)
{
	return sceKernelDeleteSema(mutexid);
}	

int sceKernelUnlockMutexPatched(SceUID mutexid, int unlockCount)
{
	return sceKernelSignalSema(mutexid, 1);
}

int sceKernelTryLockMutexPatched(SceUID mutexid, int lockCount)
{
	return sceKernelPollSema(mutexid, 1);
}

int sceKernelLockMutexPatched(SceUID mutexid, int lockCount, unsigned int *timeout)
{
	int thid = sceKernelGetThreadId();
	int res = 0;
	if (thid != last_thid) {
		res = sceKernelWaitSema(mutexid, 1, timeout);
		last_thid = thid;
	}
	return res;
}

SceUID sceKernelCreateMutexPatched(const char *name, SceUInt attr, int initCount, s32 *option)
{
	return sceKernelCreateSema(name, 0, initCount == 0, 1, option);
}

void PatchSceLoaderCore()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceLoaderCoreTool");
	pspSdkInstallNoDeviceCheckPatch();
	pspSdkInstallNoPlainModuleCheckPatch();
	MAKE_CALL(mod->text_addr + 0x154c, doLinkLibraryEntriesPatched);
	doLinkLibraryEntries = mod->text_addr + 0x136c;
}

int module_start(SceSize args, void *argp)
{
	PatchSceLoaderCore();
	PatchSceModuleManager();
	SetModuleStartUpHandler(OnModuleStart);
	ClearCaches();

	return 0;
}