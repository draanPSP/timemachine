#include <string.h>

#include <pspiofilemgr_kernel.h>
#include <pspsysevent.h>
#include <pspthreadman_kernel.h>
#include <pspsysmem_kernel.h>

#if PSP_FW_VERSION > 300 || defined FLASH_EMU_FLASH2
#include <pspinit.h>

#define PSP_INIT_KEYCONFIG_APP 0x400
#define PSP_BOOT_FLASH3 0x80
#endif

#include "flashemu.h"

#define MS0 "ms0:"

char path[260];

#if defined FLASH_EMU_HEAP_FREED_FIX || defined FLASH_EMU_TOO_MANY_FILES_FIX
#define TRACK_OPEN_FILES
FileHandler file_handler[MAX_FILES];
#endif

#if defined FLASH_EMU_TOO_MANY_FILES_FIX && PSP_FW_VERSION < 660
int (* df_open)(s32 a0, char* path, s32 a2, s32 a3);
int (* df_dopen)(s32 a0, char* path, s32 a2);
int (* df_devctl)(s32 a0, s32 a1, s32 a2, s32 a3);
#endif

SceUID flashemu_sema;
SceUID flashemuThid;

int msNotReady = 1;
int installed = 0;

extern PspSysEventHandler sysEventHandler;

#define Lock() sceKernelWaitSema(flashemu_sema, 1, NULL)
#define UnLock() sceKernelSignalSema(flashemu_sema, 1)

#define SCE_ERROR_ERRNO150_ENOTSUP    0x8001B000
#define SCE_ERROR_ERRNO_NOT_SUPPORTED 0x80010086

static int FlashEmu_IoInit();
static int FlashEmu_IoExit();
static int FlashEmu_IoOpen(PspIoDrvFileArg *arg, char *file, int flags, SceMode mode);
static int FlashEmu_IoClose(PspIoDrvFileArg *arg);
static int FlashEmu_IoRead(PspIoDrvFileArg *arg, char *data, int len);
static int FlashEmu_IoWrite(PspIoDrvFileArg *arg, const char *data, int len);
static SceOff FlashEmu_IoLseek(PspIoDrvFileArg *arg, SceOff ofs, int whence);
static int FlashEmu_IoIoctl(PspIoDrvFileArg *arg, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
static int FlashEmu_IoRemove(PspIoDrvFileArg *arg, const char *name);
static int FlashEmu_IoMkdir(PspIoDrvFileArg *arg, const char *name, SceMode mode);
static int FlashEmu_IoRmdir(PspIoDrvFileArg *arg, const char *name);
static int FlashEmu_IoDopen(PspIoDrvFileArg *arg, const char *dirName);
static int FlashEmu_IoDclose(PspIoDrvFileArg *arg);
static int FlashEmu_IoDread(PspIoDrvFileArg *arg, SceIoDirent *dir);
static int FlashEmu_IoGetstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat);
static int FlashEmu_IoChstat(PspIoDrvFileArg *arg, const char* file, SceIoStat *stat, int bits);
static int FlashEmu_IoRename(PspIoDrvFileArg *arg, const char * oldname, const char *newname);
static int FlashEmu_IoChdir(PspIoDrvFileArg *arg, const char *dir);
static int FlashEmu_IoMount();
static int FlashEmu_IoUmount();
static int FlashEmu_IoDevctl(PspIoDrvFileArg *arg, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
static int FlashEmu_IoUnk21();

static int DummyReturnZero();
static int DummyReturnNotSupported();

int SceLfatfsAssign();

static PspIoDrvFuncs lflashFuncs = {
	DummyReturnZero,		 //IoInit
	DummyReturnZero,		 //IoExit
	DummyReturnZero,		 //IoOpen
	DummyReturnZero,		 //IoClose
	DummyReturnZero,		 //IoRead
	DummyReturnZero,		 //IoWrite
	DummyReturnZero,		 //IoLseek
	DummyReturnZero,		 //IoIoctl
	DummyReturnNotSupported, //IoRemove
	DummyReturnNotSupported, //IoMkdir
	DummyReturnNotSupported, //IoRmdir
	DummyReturnNotSupported, //IoDopen
	DummyReturnNotSupported, //IoDclose
	DummyReturnNotSupported, //IoDread
	DummyReturnNotSupported, //IoGetstat
	DummyReturnNotSupported, //IoChstat
	DummyReturnNotSupported, //IoRename
	DummyReturnNotSupported, //IoChdir
	DummyReturnNotSupported, //IoMount
	DummyReturnNotSupported, //IoUmount
	DummyReturnZero,		 //IoDevctl
	DummyReturnNotSupported //IoUnk21
};

static PspIoDrvFuncs flashFatFuncs = {
	FlashEmu_IoInit,
	FlashEmu_IoExit,
	FlashEmu_IoOpen,
	FlashEmu_IoClose,
	FlashEmu_IoRead,
	FlashEmu_IoWrite,
	FlashEmu_IoLseek,
	FlashEmu_IoIoctl,
	FlashEmu_IoRemove,
	FlashEmu_IoMkdir,
	FlashEmu_IoRmdir,
	FlashEmu_IoDopen,
	FlashEmu_IoDclose,
	FlashEmu_IoDread,
	FlashEmu_IoGetstat,
	FlashEmu_IoChstat,
	FlashEmu_IoRename,
	FlashEmu_IoChdir,
	FlashEmu_IoMount,
	FlashEmu_IoUmount,
	FlashEmu_IoDevctl,
	FlashEmu_IoUnk21
};

static PspIoDrv lflashDrv = {"lflash", 0x4, 0x200, 0, &lflashFuncs};
#if PSP_FW_VERSION >= 500
static PspIoDrv flashFatDrv = {"flashfat", 0x1E0010, 1, "FAT over Flash", &flashFatFuncs};
#else
static PspIoDrv flashFatDrv = {"flashfat", 0x10, 1, "FAT over USB Mass", &flashFatFuncs};
#endif

int InstallFlashEmu()
{
	if (!installed)
	{
		sceIoAddDrv(&lflashDrv);
		sceIoAddDrv(&flashFatDrv);
		sceKernelRegisterSysEventHandler(&sysEventHandler);
		flashemuThid = sceKernelCreateThread("SceLfatfsAssign", SceLfatfsAssign, 0x64, 0x1000, 0, NULL);
		if (flashemuThid < 0)
		{
			return flashemuThid;
		}
		else
		{
			installed = 1;
			return sceKernelStartThread(flashemuThid, 0, NULL);
		}
	}

	return -1;
}

int UninstallFlashEmu()
{
	return 0;
}

void WaitMS()
{
	SceUID fd;
	if (msNotReady)
	{
		while (1)
		{
			fd = sceIoOpen(MS0 TM_PATH "/nandipl.bin", 1, 0);
			if (fd >= 0)
			{
				sceIoClose(fd);
				break;
			}
#if PSP_FW_VER >= 660
			sceKernelDelayThreadCB(20000);
#else
			sceKernelDelayThread(20000);
#endif
		}

		msNotReady = 0;
	}

	return;
}

#ifdef TRACK_OPEN_FILES

int WaitFileAvailable(int index, char *path, int flags, SceMode mode)
{
	WaitMS();

	SceUID fd;

	while(1)
	{
		if(flags == DIR_FLAG)
			fd = sceIoDopen(path);
		else
			fd = sceIoOpen(path, flags, mode);

		if(fd != 0x80010018)
			break;

		int i;
		for(i = 0; i < MAX_FILES; i++)
		{
			if(file_handler[i].opened && file_handler[i].unk_8 == 0 && file_handler[i].flags == PSP_O_RDONLY)
			{
				file_handler[i].offset = sceIoLseek(file_handler[i].fd, 0, PSP_SEEK_CUR);
				sceIoClose(file_handler[i].fd);

				file_handler[i].unk_8 = 1;
			}
		}
	}

	if(fd >= 0)
	{
		file_handler[index].unk_8 = 0;
		file_handler[index].opened = 1;
		file_handler[index].fd = fd;
		file_handler[index].mode = mode;
		file_handler[index].flags = flags;

		if(file_handler[index].path != path)
			strncpy(file_handler[index].path, path, sizeof(file_handler[index].path));
	}

	return fd;
}

SceUID GetFileIdByIndex(int index)
{
	if(file_handler[index].opened == 0) return -1;

	if(file_handler[index].unk_8)
	{
		SceUID fd = WaitFileAvailable(index, file_handler[index].path, file_handler[index].flags, file_handler[index].mode);
		if(fd >= 0)
		{
			sceIoLseek(fd, file_handler[index].offset, PSP_SEEK_SET);
			file_handler[index].fd = fd;
			file_handler[index].unk_8 = 0;
			
			return file_handler[index].fd;
		}

		return fd;
	}

	return file_handler[index].fd;
}

#endif

static void BuildPath(const char *file)
{
	strcpy(path, MS0 TM_PATH);
	strcat(path, file);
}

static int FlashEmu_IoExit()
{
	return 0;
}

static int FlashEmu_IoMount()
{
	return 0;
}

static int FlashEmu_IoUmount()
{
	return 0;
}

static int FlashEmu_IoDevctl(PspIoDrvFileArg *arg, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	int res = 0;

	switch (cmd)
	{
	case 0x5802:
		res = 0;
		break;
	case 0xb803:
		res = 0;
		break;
#if PSP_FW_VERSION >= 500
	case 0x208813:
		res = 0;
		if(sceKernelGetModel() == 0 || arg->fs_num != 3)
			res = 0x80010016;
#endif

	default:
		//Kprintf("Unknown devctl command 0x%08X\n", res);
		while (1);
	}

	return res;
}

static int FlashEmu_IoUnk21(PspIoDrvFileArg *arg)
{
#if PSP_FW_VERSION == 150
	return SCE_ERROR_ERRNO150_ENOTSUP;
#else
	return SCE_ERROR_ERRNO_NOT_SUPPORTED;
#endif
}

static int DummyReturnZero()
{
	return 0;
}

static int DummyReturnNotSupported()
{
#if PSP_FW_VERSION == 150
	return SCE_ERROR_ERRNO150_ENOTSUP;
#else
	return SCE_ERROR_ERRNO_NOT_SUPPORTED;
#endif
}

static int chdir_main(int *argv)
{
	const char *path = (const char *)argv[1];

	Lock();
	BuildPath(path);

	int res = sceIoChdir(path);

	UnLock();
	return res;
}

static int FlashEmu_IoChdir(PspIoDrvFileArg *arg, const char *dir)
{
	int argv[2];
	argv[0] = (int)arg;
	argv[1] = (int)dir;

	return sceKernelExtendKernelStack(0x4000, (void *)chdir_main, (void *)argv);
}

static int rename_main(int *argv)
{
	const char *oldname = (const char *)argv[1];
	const char *newname = (const char *)argv[2];

	Lock();
	BuildPath(oldname);

	int res = sceIoRename(oldname, newname);

	UnLock();
	return res;
}

static int FlashEmu_IoRename(PspIoDrvFileArg *arg, const char *oldname, const char *newname)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)oldname;
	argv[2] = (int)newname;

	return sceKernelExtendKernelStack(0x4000, (void *)rename_main, (void *)argv);
}

static int chstat_main(int *argv)
{
	const char *file = (const char *)argv[1];
	SceIoStat *stat = (SceIoStat *)argv[2];
	int bits = argv[3];

	Lock();
	BuildPath(file);

	int res = sceIoChstat(path, stat, bits);

	UnLock();
	return res;
}

static int FlashEmu_IoChstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat, int bits)
{
	int argv[4];

	argv[0] = (int)arg;
	argv[1] = (int)file;
	argv[2] = (int)stat;
	argv[3] = (int)bits;

	return sceKernelExtendKernelStack(0x4000, (void *)chstat_main, (void *)argv);
}

static int getstat_main(int *argv)
{
	const char *file = (const char *)argv[1];
	SceIoStat *stat = (SceIoStat *)argv[2];

	Lock();
	BuildPath(file);

	int res = sceIoGetstat(path, stat);

	UnLock();

	return res;
}

static int FlashEmu_IoGetstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)file;
	argv[2] = (int)stat;

	return sceKernelExtendKernelStack(0x4000, (void *)getstat_main, (void *)argv);
}

static int dread_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	SceIoDirent *dir = (SceIoDirent *)argv[1];

	Lock();
	
#ifdef TRACK_OPEN_FILES
	SceUID fd = GetFileIdByIndex((int)arg->arg);
	if(fd >= 0)
	{
		int res = sceIoDread(fd, dir);
		UnLock();
		return res;
	}
	UnLock();

	return fd;
#else
	int res = sceIoDread((SceUID)arg->arg, dir);
	UnLock();

	return res;
#endif
}

static int dclose_main(PspIoDrvFileArg *arg)
{
	Lock();

#ifdef TRACK_OPEN_FILES
	int index = (int)arg->arg;

	SceUID fd = GetFileIdByIndex(index);
	if(fd >= 0)
	{
		int res = sceIoDclose(fd);
		file_handler[index].opened = 0;
		UnLock();
		return res;
	}

	UnLock();
	return fd;
#else
	int res = sceIoDclose((SceUID)arg->arg);
	if (res < 0)
	{
		//Kprintf("Error 0x%08X in IoDclose.\n", res);
	}
	else
	{
		res = 0;
	}

	arg->arg = (void *)-1;

	UnLock();
	return res;
#endif
}

static int FlashEmu_IoDread(PspIoDrvFileArg *arg, SceIoDirent *dir)
{
	int argv[2];

	argv[0] = (int)arg;
	argv[1] = (int)dir;

	return sceKernelExtendKernelStack(0x4000, (void *)dread_main, (void *)argv);
}

static int FlashEmu_IoDclose(PspIoDrvFileArg *arg)
{
	return sceKernelExtendKernelStack(0x4000, (void *)dclose_main, (void *)arg);
}

static int dopen_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	const char *dirname = (const char *)argv[1];

	Lock();
	BuildPath(dirname);
	
#ifdef TRACK_OPEN_FILES
	int i;
	for(i = 0; i < MAX_FILES; i++)
	{		
		if(file_handler[i].opened == 0)
		{
			SceUID fd = WaitFileAvailable(i, path, DIR_FLAG, 0);
			if(fd >= 0)
			{
				arg->arg = (void *)i;
				UnLock();
				return 0;
			}

			UnLock();
			return fd;
		}
	}

	UnLock();
	return 0x80010018;
#else
	int dfd = sceIoDopen(path);
	if (dfd < 0)
	{
		UnLock();
		return dfd;
	}

	arg->arg = (void *)dfd;

	UnLock();
	return 0;
#endif
}

static int FlashEmu_IoDopen(PspIoDrvFileArg *arg, const char *dirName)
{
	int argv[2];

	argv[0] = (int)arg;
	argv[1] = (int)dirName;

	return sceKernelExtendKernelStack(0x4000, (void *)dopen_main, (void *)argv);
}

static int rmdir_main(int *argv)
{
	const char *name = (const char *)argv[1];

	Lock();
	BuildPath(name);

	int res = sceIoRmdir(path);

	UnLock();
	return res;
}

static int FlashEmu_IoRmdir(PspIoDrvFileArg *arg, const char *name)
{
	int argv[2];

	argv[0] = (int)arg;
	argv[1] = (int)name;

	return sceKernelExtendKernelStack(0x4000, (void *)rmdir_main, (void *)argv);
}

static int mkdir_main(u32 *argv)
{
	const char *name = (const char *)argv[1];
	SceMode mode = (SceMode)argv[2];

	Lock();
	BuildPath(name);

	int res = sceIoMkdir(path, mode);

	UnLock();
	return res;
}


static int FlashEmu_IoMkdir(PspIoDrvFileArg *arg, const char *name, SceMode mode)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)name;
	argv[2] = (int)mode;

	return sceKernelExtendKernelStack(0x4000, (void *)mkdir_main, (void *)argv);
}

static int remove_main(u32 *argv)
{
	const char *file = (const char *)argv[1];

	Lock();
	BuildPath(file);

	int res = sceIoRemove(path);

	UnLock();
	return res;
}

static int FlashEmu_IoRemove(PspIoDrvFileArg *arg, const char *name)
{
	int argv[2];

	argv[0] = (int)arg;
	argv[1] = (int)name;

	return sceKernelExtendKernelStack(0x4000, (void *)remove_main, (void *)argv);
}

static int lseek_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	int ofs = (int)argv[1];
	int whence = argv[2];

	Lock();
	
#ifdef TRACK_OPEN_FILES
	SceUID fd = GetFileIdByIndex((int)arg->arg);
	if(fd >= 0)
	{
		int res = sceIoLseek(fd, ofs, whence);
		UnLock();
		return res;
	}

	UnLock();
	return fd;
#else
	int res = sceIoLseek((SceUID)arg->arg, ofs, whence);
	UnLock();

	return res;
#endif
}

SceOff FlashEmu_IoLseek(PspIoDrvFileArg *arg, SceOff ofs, int whence)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)ofs;
	argv[2] = (int)whence;

	return (SceOff)sceKernelExtendKernelStack(0x4000, (void *)lseek_main, (void *)argv);
}

static int write_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	const char *data = (const char *)argv[1];
	int len = argv[2];
	int res;

	Lock();
	
#ifdef TRACK_OPEN_FILES
	SceUID fd = GetFileIdByIndex((int)arg->arg);
	if(fd >= 0)
	{
		if(!data && len == 0)
		{
			UnLock();
			return 0;
		}

		int written = sceIoWrite(fd, data, len);
		UnLock();
		return written;
	}

	UnLock();
	return fd;
#else
	if ((SceUID)arg->arg >= 0)
	{
		res = sceIoWrite((SceUID)arg->arg, data, len);
	}
	else
	{
		res = -1;
	}
	UnLock();

	return res;
#endif
}

static int FlashEmu_IoWrite(PspIoDrvFileArg *arg, const char *data, int len)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)data;
	argv[2] = (int)len;

	return sceKernelExtendKernelStack(0x4000, (void *)write_main, (void *)argv);
}

static int read_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	char *data = (char *)argv[1];
	int len = argv[2];
	
	Lock();

#ifdef TRACK_OPEN_FILES
	SceUID fd = GetFileIdByIndex((int)arg->arg);
	if(fd >= 0)
	{
		int read = sceIoRead(fd, data, len);
		UnLock();
		return read;
	}

	UnLock();
	return fd;
#else
	int res;
	if ((SceUID)arg->arg >= 0)
	{
		res = sceIoRead((SceUID)arg->arg, data, len);
	}
	else
	{
		res = -1;
	}

	UnLock();
	return res;
#endif
}

static int FlashEmu_IoRead(PspIoDrvFileArg *arg, char *data, int len)
{
	int argv[3];

	argv[0] = (int)arg;
	argv[1] = (int)data;
	argv[2] = (int)len;

	return sceKernelExtendKernelStack(0x4000, (void *)read_main, (void *)argv);
}

static int open_main(int *argv)
{
	PspIoDrvFileArg *arg = (PspIoDrvFileArg *)argv[0];
	const char *file = (const char *)argv[1];
	int flags = argv[2];
	SceMode mode = (SceMode)argv[3];

	Lock();
	
	BuildPath(file);
	
#ifdef TRACK_OPEN_FILES
	int i;
	for(i = 0; i < MAX_FILES; i++)
	{
		if(file_handler[i].opened == 0)
		{
			SceUID fd = WaitFileAvailable(i, path, flags, mode);
			if(fd >= 0)
			{
				arg->arg = (void *)i;
				UnLock();
				return 0;
			}

			UnLock();
			return fd;
		}
	}

	UnLock();
	return 0x80010018;
#else

	SceUID fd = sceIoOpen(path, flags, mode);
	arg->arg = (void *)fd;
	if (fd < 0)
	{
		UnLock();
		return -1;
	}

	UnLock();

	return 0;
#endif
}

static int FlashEmu_IoOpen(PspIoDrvFileArg *arg, char *file, int flags, SceMode mode)
{
	int argv[4];

	argv[0] = (int)arg;
	argv[1] = (int)file;
	argv[2] = (int)flags;
	argv[3] = (int)mode;

	return sceKernelExtendKernelStack(0x4000, (void *)open_main, (void *)argv);
}

static int close_main(PspIoDrvFileArg *arg)
{
	int res = 0;

#ifdef TRACK_OPEN_FILES
	int index = (int)arg->arg;

	SceUID fd = GetFileIdByIndex(index);
	if(fd < 0)
		return fd;
	
	int ret = sceIoClose(fd);
	if(ret < 0)
		return ret;

	file_handler[index].opened = 0;
#else
	if ((SceUID)arg->arg >= 0)
	{
		res = sceIoClose((SceUID)arg->arg);
	}
	else
	{
		res = 0;
	}
	arg->arg = (void *)-1;

	if (res < 0)
	{
		return res;
	}
#endif

	return 0;
}

static int FlashEmu_IoClose(PspIoDrvFileArg *arg)
{
	Lock();
	int res = sceKernelExtendKernelStack(0x4000, (void *)close_main, (void *)arg);
	UnLock();

	return res;
}

static int FlashEmu_IoIoctl(PspIoDrvFileArg *arg, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	int res;

	//Kprintf("Ioctl cmd 0x%08X\n", cmd);

	Lock();

	switch (cmd)
	{
	case 0x00008003: // FW1.50
	case 0x00208003: // FW2.00 load prx  "/vsh/module/opening_plugin.prx"

		res = 0;

		break;

	case 0x00208006: // load prx
		res = 0;
		break;

	case 0x00208007: // after start FW2.50
		res = 0;
		break;

	case 0x00208081: // FW2.00 load prx "/vsh/module/opening_plugin.prx"
		res = 0;
		break;

	case 0x00208082:	  // FW2.80 "/vsh/module/opening_plugin.prx"
		res = 0x80010016; // opening_plugin.prx , mpeg_vsh,prx , impose_plugin.prx
		break;

	case 0x00005001: // vsh_module : system.dreg / system.ireg
		// Flush
		//res = sceKernelExtendKernelStack(0x4000, (void *)close_main, (void *)arg);
		res = 0;
		break;

	default:
		//Kprintf("Unknow ioctl 0x%08X\n", cmd);
		res = 0xffffffff;
	}

	UnLock();
	return res;
}

static int FlashEmu_IoInit()
{
	flashemu_sema = sceKernelCreateSema("FlashSema", 0, 1, 1, NULL);

	return 0;
}

#ifdef FLASH_EMU_HEAP_FREED_FIX
int (*_free_heap_all)(void);

int free_heap_all_hook(void)
{
	for(int i = 0; i < MAX_FILES; i++)
	{
		if(file_handler[i].opened && file_handler[i].unk_8 == 0 && file_handler[i].flags != DIR_FLAG)
		{
			file_handler[i].offset = sceIoLseek(file_handler[i].fd, 0, PSP_SEEK_CUR);
			file_handler[i].unk_8 = 1;
			sceIoClose(file_handler[i].fd);
		}
	}

	return _free_heap_all();
}
#endif

#ifdef FLASH_EMU_TOO_MANY_FILES_FIX
int CloseOpenFile(int *argv)
{
	Lock();
	int i;

	for(i = 0; i < MAX_FILES; i++)
	{
		if(file_handler[i].opened && file_handler[i].unk_8 == 0 && file_handler[i].flags == PSP_O_RDONLY)
		{
			file_handler[i].offset = sceIoLseek(file_handler[i].fd, 0, PSP_SEEK_CUR);
			sceIoClose(file_handler[i].fd);

			file_handler[i].unk_8 = 1;

			UnLock();
			return 0;
		}
	}

	UnLock();

	return 0x80010018;
}

#if PSP_FW_VERSION < 660
int df_dopenPatched(s32 a0, char* path, s32 a2)
{
	while(1) {
		int res = df_dopen(a0, path, a2);
		if (res != 0x80010018)
			return res;

		char* p = strstr(path, MS0 TM_PATH);
		if (p != 0)
			break;

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);
		if (res < 0)
			return res;
	}
	return 0x80010018;
}

int df_openPatched(s32 a0, char* path, s32 a2, s32 a3)
{
	while(1) {
		int res = df_open(a0, path, a2, a3);
		if (res != 0x80010018)
			return res;

		char * p = strstr(path, MS0 TM_PATH);
		if (p != 0)
			break;

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);
		if (res < 0)
			return res;
  }
  return 0x80010018;
}

int df_devctlPatched(s32 a0, s32 a1, s32 a2, s32 a3)
{
	int res;

	while(1)
	{
		res = df_devctl(a0, a1, a2, a3);
		if (res != 0x80010018) {
			return res;
		}

		res = sceKernelExtendKernelStack(0x4000, (void *)CloseOpenFile, 0);

		if (res < 0)
			break;
	}

	return res;
}
#endif
#endif

#if PSP_FW_VERSION >= 500
int sceLFatFsDevkitVersion()
{
#if PSP_FW_VERSION == 661
	return 0x6060110;
#elif PSP_FW_VERSION == 500
	return 0x05000010;
#else
#error
#endif
}

int sceLFatFs_driver_F1FBA85F()
{
	return 0;
}
#endif

#if PSP_FW_VERSION >= 660
int sceLFatFs_driver_BED8D616()
{
	return 0;
}

int sceLFatFs_driver_E63DDEB5;
#endif

void sceLfatfsWaitReady()
{
	if (flashemuThid >= 0)
		sceKernelWaitThreadEnd(flashemuThid, 0);

	return;
}

int SceLfatfsAssign()
{
	WaitMS();
	sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", IOASSIGN_RDONLY, 0, 0);
	sceIoAssign("flash1:", "lflash0:0,1", "flashfat1:", IOASSIGN_RDWR, 0, 0);

#if PSP_FW_VERSION >= 300 || defined FLASH_EMU_FLASH2
	int appType = sceKernelInitKeyConfig();

	if (appType == PSP_INIT_KEYCONFIG_VSH || appType == PSP_INIT_KEYCONFIG_GAME || appType == PSP_INIT_KEYCONFIG_APP) {
		sceIoAssign("flash2:","lflash0:0,2","flashfat2:", IOASSIGN_RDWR, 0, 0);
	}
#endif

#if PSP_FW_VERSION >= 500
	if (sceKernelGetModel() != 0)
	{
		if (appType == PSP_INIT_KEYCONFIG_VSH ||
			((appType == PSP_INIT_KEYCONFIG_GAME || appType == PSP_INIT_KEYCONFIG_APP) && sceKernelBootFrom() == PSP_BOOT_FLASH3)) {
			sceIoAssign("flash3:","lflash0:0,3","flashfat3:", IOASSIGN_RDWR, 0, 0);
		}
	}
#endif

	flashemuThid = -1;
	sceKernelExitDeleteThread(0);

	return 0;
}

int sceLfatfsStop()
{
	return 0;
}