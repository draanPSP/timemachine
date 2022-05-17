#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <pspkernel.h>
#include <pspreg.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include <pspmacro.h>
#include <plainModulesPatch.h>

#include "main.h"
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

int last_br;
int last_unk;

int (* GetMsSize)(void);

int start_thread;
static SceModule2 *last_module;

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

u32 GetModuleExportFuncAddr(char *moduleName, char *libraryName, int nid)
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName(moduleName);
	if (mod != NULL)
	{
		SceLibraryEntryTable *libEntryTable = mod->ent_top;
		int i;

		if (mod->ent_size <= 0)
			return 0;

		while ((u32)libEntryTable < ((u32)mod->ent_top + mod->ent_size))
		{
			if (strcmp(libEntryTable->libname, libraryName) == 0)
			{
				/* Find the specifed NID and it's offset with the entry table */
				for (i = 0; i < libEntryTable->stubcount; i++)
				{
					if (((int *)libEntryTable->entrytable)[i] == nid)
					{
						return ((u32 *)libEntryTable->entrytable)[libEntryTable->stubcount + libEntryTable->vstubcount + i];
					}
				}
			}
			libEntryTable = (SceLibraryEntryTable *)((u32)libEntryTable + (libEntryTable->len * 4));
		}
	}
	return 0;
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

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	sceKernelGzipDecompress((u8 *)0x88FC0000, 0x10000, rebootex_01g, 0);

	return sceKernelLzrcDecode(dest, destSize, src, unk);
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

void PatchSceLoadExec()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceLoadExec");
	if (mod != NULL)
	{
		MAKE_CALL(mod->text_addr + 0x1738, RebootBinDecompressPatched);
		_sw(0x3C0188FC, mod->text_addr + 0x1774); //LUI $at 0x88FC - Patch call to reboot.bin

		ClearCaches();
	}
}

void PatchSceModuleManager()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceModuleManager");
	
	if (mod != NULL)
	{
		MAKE_JUMP(mod->text_addr + 0x65dc, sceKernelCreateThreadPatched);
		MAKE_JUMP(mod->text_addr + 0x661c, sceKernelStartThreadPatched);
		ClearCaches();
	}
}

int OnModuleStart(SceModule2 *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName, "sceLflashFatfmt") == 0)
	{
		u32 funcAddr = GetModuleExportFuncAddr("sceLflashFatfmt", "LflashFatfmt", 0xB7A424A4); //sceLflashFatfmtStartFatfmt
		if (funcAddr)
		{
			MAKE_FUNCTION_RETURN0(funcAddr);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "scePower_Service") == 0)
	{
		// Enable 4th level brightness when power not connected. Ignore result from call to scePowerIsPowerOnline
		_sw(0, text_addr+0xb24);
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceLoadExec") == 0)
	{
		PatchSceLoadExec();
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceMSstor_Driver") == 0)
	{
		REDIRECT_FUNCTION(text_addr + 0xa00, ValidateSeekPatched);
		REDIRECT_FUNCTION(text_addr + 0xa88, ValidateSeekP1Patched);
		GetMsSize = (void *)(text_addr + 0x34fc);

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceMSFAT_Driver") == 0)
	{
		// Remove validation in df_open checking data pointer not null
		// Causes issues with registry init in vsh
		_sw(0, text_addr + 0xe34);

		// Fix get_stat failure
		_sw(0, text_addr + 0x21f8);

		ClearCaches();
	}

	return 0;
}

void PatchLoadCore()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceLoaderCore");

	if (mod != NULL) {

		//Restore first 2 instructions of sceKernelCheckExecFile
		_sw(_lw(mod->text_addr + 0x2808), mod->text_addr + 0x36e4);
		_sw(_lw(mod->text_addr + 0x280c), mod->text_addr + 0x36e8);

		KERNEL_HIJACK_FUNCTION(mod->text_addr + 0x36e4, sceKernelCheckExecFileHook, _sceKernelCheckExecFile);

		ClearCaches();
	}
}

int module_start(SceSize args, void *argp)
{
	PatchLoadCore();
	PatchSceModuleManager();
	PatchSceLoadExec();
	SetModuleStartUpHandler(OnModuleStart);
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

extern SceUID flashemu_sema;

#define Lock() sceKernelWaitSema(flashemu_sema, 1, NULL)
#define UnLock() sceKernelSignalSema(flashemu_sema, 1)

int SysEventHandler(int eventId, char *eventName, void *param, int *result)
{
	if (eventId == 0x10009) //resume
		msNotReady = 1;
	return 0;
}
