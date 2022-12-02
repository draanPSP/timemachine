#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <pspkernel.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>
#include <pspthreadman_kernel.h>
#include <pspsysmem_kernel.h>

#include <pspmacro.h>
#include <moduleUtils.h>
#include <flashemu.h>

#include <systemctrl.h>
#include <rebootex_01g.h>
#include <rebootex_02g.h>

PSP_MODULE_INFO("TimeMachine_Control", PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_START | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_NO_STOP, 1, 0);

int SysEventHandler(int eventId, char *eventName, void *param, int *result);

PspSysEventHandler sysEventHandler =
	{
		.size = sizeof(PspSysEventHandler),
		.name = "",
		.type_mask = 0x00FFFF00,
		.handler = SysEventHandler};

extern SceUID flashemu_sema;
extern int msNotReady;
extern FileHandler file_handler[MAX_FILES];

extern int CloseOpenFile(int *argv);

STMOD_HANDLER previous;

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	if ((u32)dest == 0x88fc0000)
	{
		int model = sceKernelGetModel();
		if (model == 0)
			src = (char *)rebootex_01g;
		else if (model == 1)
			src = (char *)rebootex_02g;

		destSize = 0x10000;
	}

	return sceKernelGzipDecompress(dest, destSize, src, 0);
}

int codePagesceIoOpenPatched(const char *file, int flags, SceMode mode)
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("vsh_module");

	if (!mod)
		return 0x80010018;

	return sceIoOpen(file, flags, mode);
}

int df_dopenPatched(int type, void * cb, void *arg)
{
	int res;

	while(1) {
		res = sceKernelExtendKernelStack(type, cb, arg);
		if (res != 0x80010018)
			return res;

		if (*(int *)(arg + 4) == 0)
			continue;

		if (memcmp((void *)(*(int *)(arg + 4) + 4), TM_PATH_W, sizeof(TM_PATH_W)) == 0)
			continue;

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);
		if (res < 0)
			break;
	}
	return res;
}

int df_openPatched(int type, void * cb, void *arg)
{
	int res;

	while(1) {
		res = sceKernelExtendKernelStack(type, cb, arg);
		if (res != 0x80010018)
			return res;

		if (*(int *)(arg + 4) == 0)
			continue;

		if (memcmp((void *)(*(int *)(arg + 4) + 4), TM_PATH_W, sizeof(TM_PATH_W)) == 0)
			continue;

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);
		if (res < 0)
			break;
	}
	return res;
}

int df_devctlPatched(int type, void *cb, void *arg)
{
	int res;

	while(1)
	{
		res = sceKernelExtendKernelStack(type, cb, arg);
		if (res != 0x80010018)
			break;

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);

		if (res < 0)
			break;
	}

	return res;
}

int OnModuleStart(SceModule2 *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName, "sceUtility_Driver") == 0)
	{
		SceModule2 *mod2 = (SceModule2 *)sceKernelFindModuleByName("sceMSFAT_Driver");

		MAKE_CALL(mod2->text_addr + 0x30fc, df_openPatched);
		MAKE_CALL(mod2->text_addr + 0x3ba4, df_dopenPatched);
		MAKE_CALL(mod2->text_addr + 0x44cc, df_devctlPatched);

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceLflashFatfmt") == 0)
	{
		u32 funcAddr = sctrlHENFindFunction("sceLflashFatfmt", "LflashFatfmt", 0xb7a424a4); // sceLflashFatfmtStartFatfmt
		if (funcAddr)
		{
			MAKE_FUNCTION_RETURN0(funcAddr);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "sceCodepage_Service") == 0)
	{
		u32 stubAddr = GetModuleImportFuncAddr("sceCodepage_Service", "IoFileMgrForKernel", 0x109f50bc); // sceIoOpen
		if (stubAddr)
			REDIRECT_FUNCTION(stubAddr, codePagesceIoOpenPatched);

		ClearCaches();
	}

	if (!previous)
		return 0;

	return previous(mod);
}

void PatchSystemControl()
{
	// Patch import stub of sceKernelGzipDecompress
	MAKE_JUMP((u32)GetModuleImportFuncAddr("SystemControl", "UtilsForKernel", 0x78934841), RebootBinDecompressPatched);
}

void PatchLoadCore()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceLoaderCore");

	//Restore nand decrypt function call
	MAKE_CALL(mod->text_addr + 0x5994, mod->text_addr + 0x7824);
}

int module_start(SceSize args, void *argp)
{
	PatchSystemControl();
	InstallFlashEmu();
	previous = sctrlHENSetStartModuleHandler(OnModuleStart);
	PatchLoadCore();
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
	sceIoUnassign("flash2:");
	sceIoUnassign("flash3:");
	sceKernelUnregisterSysEventHandler(&sysEventHandler);

	return 0;
}

int SysEventHandler(int eventId, char *eventName, void *param, int *result)
{
	if (eventId == 0x4000) //suspend
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
	else if (eventId == 0x10009) // resume
		msNotReady = 1;
	return 0;
}
