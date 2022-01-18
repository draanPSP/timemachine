#include <string.h>

#include <pspidstorage.h>
#include <pspkernel.h>
#include <pspreg.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include "main.h"
#include "flashemu.h"
#include <rebootex.h>

PSP_MODULE_INFO("TimeMachine_Control", PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_START | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_NO_STOP, 1, 0);

#define JAL_OPCODE 0x0C000000
#define J_OPCODE 0x08000000
#define SC_OPCODE 0x0000000C
#define JR_RA 0x03e00008

#define NOP 0x00000000

#define MAKE_CALL(a, f) _sw(JAL_OPCODE | (((u32)(f)&0x3FFFFFFF) >> 2), a);
#define MAKE_JUMP(a, f) _sw(J_OPCODE | (((u32)(f)&0x3FFFFFFF) >> 2), a);
#define MAKE_SYSCALL(a, n) _sw(SC_OPCODE | (n << 6), a);
#define JUMP_TARGET(x) ((x & 0x3FFFFFFF) << 2)
#define REDIRECT_FUNCTION(a, f)                        \
	_sw(J_OPCODE | (((u32)(f) >> 2) & 0x03ffffff), a); \
	_sw(NOP, a + 4);
#define MAKE_DUMMY_FUNCTION0(a) \
	_sw(0x03e00008, a);         \
	_sw(0x00001021, a + 4);
#define MAKE_DUMMY_FUNCTION1(a) \
	_sw(0x03e00008, a);         \
	_sw(0x24020001, a + 4);

int sceDisplaySetBrightnessPatched(int level, int unk1);
int SysEventHandler(int eventId, char *eventName, void *param, int *result);

PspSysEventHandler sysEventHandler =
	{
		.size = sizeof(PspSysEventHandler),
		.name = "",
		.type_mask = 0x0000FF00,
		.handler = SysEventHandler};

extern SceUID flashemu_sema;
extern int msNotReady;

int last_br;
int last_unk;

int (* GetMsSize)(void);

int start_thread;
static SceModule2 *last_module;

STMOD_HANDLER stmod_handler = NULL;

//SystemControl150
u8* rebootbin340;
u8* oerebootex;
int* isReboot;

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

int RebootBinDecompressPatched(u8 *dest, int destSize, u8 *src, int unk)
{
	// Copy oe 150 rebootex.bin to 0x88FC0000
	memcpy(0x88fc0000, oerebootex + 0x10, 0xa38);

	sceKernelGzipDecompress(0x88fd0000, 0x10000, rebootex_01g, 0);

	if (*isReboot)
		return sceKernelDeflateDecompress(0x88600000, destSize, rebootbin340 + 0x10, unk);

	return sceKernelGzipDecompress(dest, destSize, src, unk);
}

void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

void PatchSceModuleManager()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceModuleManager");

	MAKE_JUMP(mod->text_addr + 0x4714, sceKernelCreateThreadPatched);
	MAKE_JUMP(mod->text_addr + 0x4724, sceKernelStartThreadPatched);
}

int OnModuleStart(SceModule2 *mod)
{
	char *moduleName = mod->modname;
	u32 text_addr = mod->text_addr;

	if (strcmp(moduleName, "sceRegistry_Service") == 0)
	{
		_sw((u32)sceRegGetKeyInfoPatched, text_addr+0x76dc);
		_sw((u32)sceRegGetKeyValuePatched, text_addr+0x76e0);

		ClearCaches();
	}
	else if (strcmp(moduleName, "SystemControl150") == 0)
	{
		_sw(0x344288fd, text_addr + 0x1ac); //LUI $v0, 0x88fd - Patch call to oe rebootex.bin

		REDIRECT_FUNCTION(text_addr + 0x1c, RebootBinDecompressPatched);

		rebootbin340 = text_addr + 0x830;
		oerebootex = text_addr + 0xb908;
		isReboot = text_addr + 0xc350;

		ClearCaches();
	}
	else if (strcmp(moduleName, "sceLflashFatfmt") == 0)
	{
		u32 funcAddr = GetModuleExportFuncAddr("sceLflashFatfmt", "LflashFatfmt", 0xB7A424A4); //sceLflashFatfmtStartFatfmt
		if (funcAddr)
		{
			MAKE_DUMMY_FUNCTION0(funcAddr);
			ClearCaches();
		}
	}
	else if (strcmp(moduleName, "scePower_Service") == 0)
	{
		_sw(0x00e02021, text_addr + 0x558); //ADDU $a0 $a3 $zero
		ClearCaches();
	}
	else if (strcmp(moduleName, "sceDisplay_Service") == 0)
	{
		int lcd;
		if (sceIdStorageLookup(8, 0, &lcd, 4) >= 0 && lcd == 0x4C434470)
		{
			_sw((u32)sceDisplaySetBrightnessPatched, text_addr + 0x2858);
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

	return 0;
}

int module_start(SceSize args, void *argp)
{
	PatchSceModuleManager();

	if (sceKernelFindModuleByName("lcpatcher") == 0)
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
	if (eventId == 0x10009) //resume
		msNotReady = 1;
	else if (eventId == 0x1000b)
	{
		if (last_br == 100)
			sceDisplaySetBrightnessPatched(last_br, last_unk);
	}

	return 0;
}
