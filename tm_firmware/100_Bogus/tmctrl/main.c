#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <pspidstorage.h>
#include <pspkernel.h>
#include <pspreg.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include <pspmacro.h>
#include <moduleUtils.h>
#include <flashemu.h>

#include <rebootex.h>

PSP_MODULE_INFO("TimeMachine_Control", PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_START | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_NO_STOP, 1, 0);

int SysEventHandler(int eventId, char *eventName, void *param, int *result);

PspSysEventHandler sysEventHandler =
	{
		.size = sizeof(PspSysEventHandler),
		.name = "",
		.type_mask = 0x00FFFF00,
		.handler = SysEventHandler
	};

extern SceUID flashemu_sema;
extern int msNotReady;

extern int (*_free_heap_all)(void);
extern int free_heap_all_hook(void);

int last_br;
int last_unk;

int (* GetMsSize)(void);

int start_thread;
static SceModule *last_module;
u32 sceRebootTextAddr;

typedef int (* STMOD_HANDLER)(SceModule *);
STMOD_HANDLER stmod_handler = NULL;

STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER handler)
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
		last_module = (SceModule *)sceKernelFindModuleByAddress((u32)entry);
	}

	return thid;
}

REGHANDLE lang_hk = -1;

int sceRegGetKeyValuePatched(REGHANDLE hd, REGHANDLE hk, void *buffer, SceSize size)
{
	int res = sceRegGetKeyValue(hd, hk, buffer, size);
	if (res >= 0 && hk == lang_hk)
	{
		if (*(u32 *)buffer > 2)
		{
			*(u32 *)buffer = 1;
		}

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

int sceKernelRebootBeforeForUserPatched(int a0)
{
	sceKernelGzipDecompress((u8 *)0x88FC0000, 0x10000, rebootex_01g, 0);

	return sceKernelRebootBeforeForUser(a0);
}

int sceKernelRebootPatched(int *a0, int *a1, int* a2)
{
	void (*rebootex_entry)(int *a0, int *a1, int* a2, u32 sceRebootTextAddr) = (void *)0x88fc0000;

	//Jump to rebootex
	rebootex_entry(a0, a1, a2, sceRebootTextAddr);

	__builtin_unreachable();
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

void PatchSceLoadExec()
{
	SceModule *mod = (SceModule *)sceKernelFindModuleByName("sceLoadExec");
	if (mod != NULL)
	{
		MAKE_CALL(mod->text_addr + 0xef4, sceKernelRebootBeforeForUserPatched);

		//Patch call to sceKernelReboot
		MAKE_CALL(mod->text_addr + 0x109c, sceKernelRebootPatched);
	}
}

void PatchSceModuleManager()
{
	SceModule *mod = (SceModule *)sceKernelFindModuleByName("sceModuleManager");
	
	MAKE_JUMP(mod->text_addr + 0x46b4, sceKernelCreateThreadPatched);
	MAKE_JUMP(mod->text_addr + 0x46c4, sceKernelStartThreadPatched);
}

int OnModuleStart(SceModule *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName, "sceRegistry_Service") == 0)
	{
		_sw((u32)sceRegGetKeyInfoPatched, text_addr+0x7264);
		_sw((u32)sceRegGetKeyValuePatched, text_addr+0x7268);

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceLflashFatfmt") == 0)
	{
		u32 funcAddr = GetModuleExportFuncAddr("sceLflashFatfmt", "LflashFatfmt", 0xb7a424a4); //sceLflashFatfmtStartFatfmt
		if (funcAddr)
		{
			MAKE_FUNCTION_RETURN0(funcAddr);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "sceLoadExec") == 0)
	{
		PatchSceLoadExec();
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceMSFAT_Driver") == 0) {
		KERNEL_HIJACK_FUNCTION(text_addr + 0x0728, free_heap_all_hook, _free_heap_all);
	}
	else if (strcmp(moduleName, "sceReboot") == 0) {
		sceRebootTextAddr = text_addr;
	}

	return 0;
}

int module_start(SceSize args, void *argp)
{
	PatchSceModuleManager();
	PatchSceLoadExec();
	sctrlHENSetStartModuleHandler(OnModuleStart);
	InstallFlashEmu();
	ClearCaches();

	return 0;
}

int module_reboot_before(SceSize args, void *argp)
{
	SceUInt timeout = 500000;
	sceKernelWaitSema(flashemu_sema, 1, &timeout);
	sceKernelDeleteSema(flashemu_sema);
	sceIoUnassign("flash0:");
	sceIoUnassign("flash1:");
	sceKernelUnregisterSysEventHandler(&sysEventHandler);

	return 0;
}

int SysEventHandler(int eventId, char *eventName, void *param, int *result)
{
	if (eventId == 0x10009) //resume
		msNotReady = 1;

	return 0;
}
