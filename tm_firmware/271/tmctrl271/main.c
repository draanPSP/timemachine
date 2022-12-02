#include <string.h>

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
		.type_mask = 0x0000FF00,
		.handler = SysEventHandler};

extern SceUID flashemu_sema;
extern int msNotReady;
extern FileHandler file_handler[MAX_FILES];

extern u32 (* df_open)(s32 a0, char* path, s32 a2, s32 a3);
extern u32 (* df_dopen)(s32 a0, char* path, s32 a2);
extern u32 (* df_devctl)(s32 a0, s32 a1, s32 a2, s32 a3);

extern u32 (* df_openPatched)(s32 a0, char* path, s32 a2, s32 a3);
extern u32 (* df_dopenPatched)(s32 a0, char* path, s32 a2);
extern u32 (* df_devctlPatched)(s32 a0, s32 a1, s32 a2, s32 a3);

int start_thread;
static SceModule2 *last_module;

typedef int (* STMOD_HANDLER)(SceModule2 *);
STMOD_HANDLER stmod_handler = NULL;

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

int (*_RebootBinDecompressPatched)(u8 *dest, int destSize, u8 *src, int unk);

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	int res = _RebootBinDecompressPatched(dest, destSize, src, unk);

	sceKernelGzipDecompress((u8 *)0x88fd0000, 0x10000, rebootex_01g, 0);

	return res;
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

void PatchSystemControl()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("SystemControl");

	_sw(0x344288fd, mod->text_addr + 0x3de4); // Patch call to oe rebootex
	KERNEL_HIJACK_FUNCTION(mod->text_addr + 0x2040, RebootBinDecompressPatched, _RebootBinDecompressPatched);
}

void PatchSceModuleManager()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceModuleManager");

	MAKE_JUMP(mod->text_addr + 0x63c4, sceKernelCreateThreadPatched);
	MAKE_JUMP(mod->text_addr + 0x640c, sceKernelStartThreadPatched);
}

int OnModuleStart(SceModule2 *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName, "sceMediaSync") == 0)
	{
		SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceMSFAT_Driver");
		_sw(0, mod->text_addr + 0x21ec);
		MAKE_CALL(mod->text_addr + 0x47a8, df_openPatched);
		MAKE_CALL(mod->text_addr + 0x4930, df_dopenPatched);
		MAKE_CALL(mod->text_addr + 0x4ae0, df_devctlPatched);

		df_open = (u32 (*)(s32,  char *, s32,  s32))(mod->text_addr + 0x98c);
		df_dopen = (u32 (*)(s32,  char *, s32))(mod->text_addr + 0x1a44);
		df_devctl = (u32 (*)(s32,  s32, s32,  s32))(mod->text_addr + 0x29d8);

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

	return 0;
}

int module_start(SceSize args, void *argp)
{
	PatchSceModuleManager();
	PatchSystemControl();
	InstallFlashEmu();
	SetModuleStartUpHandler(OnModuleStart);
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
	if(eventId == 0x4000) //suspend
	{
		int i;
		for(i = 0; i < MAX_FILES; i++)
		{
			if(file_handler[i].opened && file_handler[i].unk_8 == 0 && file_handler[i].flags != DIR_FLAG)
			{
				file_handler[i].offset = sceIoLseek(file_handler[i].fd, 0, PSP_SEEK_CUR);
				file_handler[i].unk_8 = 1;
				sceIoClose(file_handler[i].fd);
			}
		}
	}
	else if (eventId == 0x10009) //resume
		msNotReady = 1;

	return 0;
}
