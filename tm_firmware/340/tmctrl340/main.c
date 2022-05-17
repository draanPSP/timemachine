#include <string.h>

#include <pspidstorage.h>
#include <pspkernel.h>
#include <pspreg.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include <pspmacro.h> 

#include "main.h"
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

APRS_EVENT previous = NULL;

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

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	sceKernelGzipDecompress((u8 *)0x88fd0000, 0x10000, rebootex_01g, 0);

	return sceKernelGzipDecompress(dest, destSize, src, unk);
}

int RebootExDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	sceKernelGzipDecompress((u8 *)0x88fd0000, 0x10000, rebootex_01g, 0);

	return sceKernelLzrcDecode(dest, destSize, src, unk);
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

int SE_GetConfig(SEConfig *config)
{
	unsigned int k1 = pspSdkSetK1(0);

	memset(config, 0, sizeof(SEConfig));
	
	SceUID fd = sceIoOpen("flash1:/config.se" , PSP_O_RDONLY, 0);
	int res = -1;

	if (fd >= 0) {
		res = sceIoRead(fd, config, sizeof(SEConfig));
		sceIoClose(fd);
	}

	pspSdkSetK1(k1);

	return res;
}

int SE_SetConfig(SEConfig *config)
{ 
	unsigned int k1 = pspSdkSetK1(0);

	SceUID fd = sceIoOpen("flash1:/config.se", 0x602, 0x1ff);
	int res = 0;

	if (fd < 0)
		return -1;

	config->magic = CONFIG_MAGIC;

	if (sceIoWrite(fd, config, sizeof(SEConfig)) < sizeof(SEConfig)) {
		res = -1;
	}

	sceIoClose(fd);
	pspSdkSetK1(k1);

	return res;
}

void PatchSystemControl()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("SystemControl");

	REDIRECT_FUNCTION(mod->text_addr + 0x5bd8, SE_GetConfig);
	REDIRECT_FUNCTION(mod->text_addr + 0x5c84, SE_SetConfig);
	_sw(0x344288fd, mod->text_addr + 0x1eb0); // Patch call to oe rebootex
	MAKE_JUMP(mod->text_addr + 0x1c10, RebootExDecompressPatched);
}

int OnPspRelSectionEvent(char *moduleName, u8 *modbuf)
{
	if (strcmp(moduleName, "Reboot150") == 0)
	{
		_sw(0x344288fd, (u32)modbuf + 0x130); //  Patch call to oe rebootex
		MAKE_JUMP((u32)modbuf + 0xa0, RebootBinDecompressPatched);

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceMediaSync") == 0)
	{
		SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceMSFAT_Driver");
		_sw(0, mod->text_addr + 0x22cc);
		MAKE_CALL(mod->text_addr + 0x4ff4, df_openPatched);
		MAKE_CALL(mod->text_addr + 0x517c, df_dopenPatched);
		MAKE_CALL(mod->text_addr + 0x532c, df_devctlPatched);

		df_open = (u32 (*)(s32,  char *, s32,  s32))(mod->text_addr + 0x8f0);
		df_dopen = (u32 (*)(s32,  char *, s32))(mod->text_addr + 0x1b4c);
		df_devctl = (u32 (*)(s32,  s32, s32,  s32))(mod->text_addr + 0x2a70);

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
	
	if (!previous)
		return 0; 

	return previous(moduleName, modbuf);
}

int module_start(SceSize args, void *argp)
{
	PatchSystemControl();
	InstallFlashEmu();
	previous = sctrlHENSetOnApplyPspRelSectionEvent(OnPspRelSectionEvent);
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
