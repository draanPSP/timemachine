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

int sceDisplaySetBrightnessPatched(int level, int unk1);
int SysEventHandler(int eventId, char *eventName, void *param, int *result);

void (*rebootex_entry)(int a0,int a1,int bootcode,int dummy) = (void *)0x88fc0000;

#define REBOOT_150_SIZE 0x16d40

char rebootbin[REBOOT_150_SIZE];

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

typedef int (* STMOD_HANDLER)(SceModule *);
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

int sceDisplaySetBrightnessPatched(int level, int unk1)
{
	last_br = level;
	last_unk = unk1;

	if (level < 100)
	{
		if (level >= 70)
			level = 100 - level;
		else if (level >= 35)
			level = 100 - level - 5;
		else
			level = 100 - level - 10;
	}
	else if (level == 100)
	{
		level = 1;
	}

	return sceDisplaySetBrightness(level, unk1);
}

int sceKernelRebootBeforeForUserPatched(void *arg)
{
	//Read 1.50 reboot.bin
	int fd = sceIoOpen("ms0:/TM/100/reboot.bin", 1, 0);

	if (fd < 0)
		return -1;

	SceOff size = sceIoLseek(fd, 0, PSP_SEEK_END);

	if (size == REBOOT_150_SIZE)
	{
		sceIoLseek(fd, 0, PSP_SEEK_SET);
		sceIoRead(fd, (void *)rebootbin, REBOOT_150_SIZE);
		sceIoClose(fd);
	}
	else
	{
		sceIoClose(fd);
		return -1;
	}

	return sceKernelRebootBeforeForUser(arg);
}

int sceKernelRebootPatched(int a0, int a1, int bootcode, int dummy)
{
	memset((void *)0x88c00000, 0, 0x400000);
	sceKernelGzipDecompress((u8 *)0x88fc0000, 0x10000, rebootex_01g, 0);
	memcpy((void *)0x88c00000, (void *)rebootbin, REBOOT_150_SIZE);

	rebootex_entry(a0, a1, bootcode, dummy);
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
		MAKE_CALL(mod->text_addr + 0xee8, sceKernelRebootBeforeForUserPatched);

		// Skip call to LoadReboot
		_sw(0, mod->text_addr + 0xf00);

		// Skip sceReboot setup
		MAKE_JUMP(mod->text_addr + 0x1094, mod->text_addr + 0x1160);

		// Patch call to sceKernelReboot
		MAKE_CALL(mod->text_addr + 0x1188, sceKernelRebootPatched);
	}
}

void PatchSceModuleManager()
{
	SceModule *mod = (SceModule *)sceKernelFindModuleByName("sceModuleManager");

	MAKE_JUMP(mod->text_addr + 0x4798, sceKernelCreateThreadPatched);
	MAKE_JUMP(mod->text_addr + 0x47a8, sceKernelStartThreadPatched);
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
	else if (strcmp(moduleName, "scePower_Service") == 0)
	{
		_sw(0x00e02021, text_addr+0x3bc); //ADDU $a0 $a3 $zero
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceDisplay_Service") == 0)
	{
		int lcd;
		if (sceIdStorageLookup(8, 0, &lcd, 4) >= 0 && lcd == 0x4c434470)
		{
			_sw((u32)sceDisplaySetBrightnessPatched, text_addr + 0x2264);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "sceLoadExec") == 0)
	{
		PatchSceLoadExec();
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceMSstor_Driver") == 0)
	{
		//TODO - Fix issues with usb connectivity
		/*REDIRECT_FUNCTION(text_addr + 0x50a0, ValidateSeekPatched);
		REDIRECT_FUNCTION(text_addr + 0x513c, ValidateSeekP1Patched);
		GetMsSize = (void *)(text_addr + 0x0288);

		ClearCaches();*/
	}
	else if (strcmp(moduleName, "sceMSFAT_Driver") == 0) {
		KERNEL_HIJACK_FUNCTION(text_addr + 0x0728, free_heap_all_hook, _free_heap_all);
	}

	return 0;
}

int module_start(SceSize args, void *argp)
{
	pspSdkInstallNoDeviceCheckPatch();
	pspSdkInstallNoPlainModuleCheckPatch();
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

int SysEventHandler(int eventId, char *eventName, void *param, int *result)
{
	if (eventId == 0x10009) //resume
		msNotReady = 1;
	else if (eventId == 0x1000b)
	{
		if (last_br == 100)
			sceDisplaySetBrightnessPatched(last_br, last_unk);
	}

	return 0;
}
