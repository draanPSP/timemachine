#include <pspkernel.h>
#include <pspsysevent.h>
#include <pspthreadman_kernel.h>
#include <pspsysmem_kernel.h>
#include <psputilsforkernel.h>
#include <systemctrl.h>

#include <stdio.h>
#include <string.h>

#include "main.h"

#include "rebootex_01g.h" // 0x229C
#include "rebootex_02g.h" // 0x3700

PSP_MODULE_INFO("TimeMachine_Control", 0x1007, 1, 0);
PSP_MAIN_THREAD_ATTR(0);

#define JAL_OPCODE	0x0C000000
#define J_OPCODE	0x08000000
#define SC_OPCODE	0x0000000C
#define JR_RA		0x03e00008

#define NOP	0x00000000

#define MAKE_CALL(a, f) _sw(JAL_OPCODE | (((u32)(f) & 0x3FFFFFFF) >> 2), a); 
#define MAKE_JUMP(a, f) _sw(J_OPCODE | (((u32)(f) & 0x3FFFFFFF) >> 2), a); 
#define MAKE_SYSCALL(a, n) _sw(SC_OPCODE | (n << 6), a);
#define JUMP_TARGET(x) ((x & 0x3FFFFFFF) << 2)
#define REDIRECT_FUNCTION(a, f) _sw(J_OPCODE | (((u32)(f) >> 2)  & 0x03ffffff), a);  _sw(NOP, a+4);
#define MAKE_DUMMY_FUNCTION0(a) _sw(0x03e00008, a); _sw(0x00001021, a+4);
#define MAKE_DUMMY_FUNCTION1(a) _sw(0x03e00008, a); _sw(0x24020001, a+4);

#define MAX_FILES 32
#define DIR_FLAG 0xD0D0

int ms_unavailable = 1; //0x4B68

//wchar_t buf_4B6C[] = L"\\TM\\DC6\\";

u8 buf_4B6C[0x10] =
{
	0x5C, 0x00, 0x54, 0x00, 0x4D, 0x00, 0x5C, 0x00,
	0x44, 0x00, 0x43, 0x00, 0x36, 0x00, 0x5C, 0x00
};

SceUID thid = -1; //0x4B7C

static struct PspSysEventHandler event_handler = //0x4B80
{
	sizeof(PspSysEventHandler),
	"",
	0x00FFFF00,
	SysEventHandler,
};

static PspIoDrvFuncs lflash_funcs = //0x4BE8
{
	.IoInit = (void *)dummy_ok,
	.IoExit = (void *)dummy_ok,
	.IoOpen = (void *)dummy_ok,
	.IoClose = (void *)dummy_ok,
	.IoRead = (void *)dummy_ok,
	.IoWrite = (void *)dummy_ok,
	.IoLseek = (void *)dummy_ok,
	.IoIoctl = (void *)dummy_ok,
	.IoRemove = (void *)dummy_error,
	.IoMkdir = (void *)dummy_error,
	.IoRmdir = (void *)dummy_error,
	.IoDopen = (void *)dummy_error,
	.IoDclose = (void *)dummy_error,
	.IoDread = (void *)dummy_error,
	.IoGetstat = (void *)dummy_error,
	.IoChstat = (void *)dummy_error,
	.IoRename = (void *)dummy_error,
	.IoChdir = (void *)dummy_error,
	.IoMount = (void *)dummy_error,
	.IoUmount = (void *)dummy_error,
	.IoDevctl = (void *)dummy_ok,
	.IoUnk21 = (void *)dummy_ok,
};

static PspIoDrv lflash_driver = //0x4BC0
{
	"lflash", 0x4, 0x200, NULL, &lflash_funcs
};

static PspIoDrvFuncs flashfat_funcs = //0x4C40
{
	.IoInit = (void *)Common_IoInit,
	.IoExit = (void *)Common_IoExit,
	.IoOpen = (void *)Common_IoOpen,
	.IoClose = (void *)Common_IoClose,
	.IoRead = (void *)Common_IoRead,
	.IoWrite = (void *)Common_IoWrite,
	.IoLseek = (void *)Common_IoLseek,
	.IoIoctl = (void *)Common_IoIoctl,
	.IoRemove = (void *)Common_IoRemove,
	.IoMkdir = (void *)Common_IoMkdir,
	.IoRmdir = (void *)Common_IoRmdir,
	.IoDopen = (void *)Common_IoDopen,
	.IoDclose = (void *)Common_IoDclose,
	.IoDread = (void *)Common_IoDread,
	.IoGetstat = (void *)Common_IoGetstat,
	.IoChstat = (void *)Common_IoChstat,
	.IoRename = (void *)Common_IoRename,
	.IoChdir = (void *)Common_IoChdir,
	.IoMount = (void *)Common_IoMount,
	.IoUmount = (void *)Common_IoUmount,
	.IoDevctl = (void *)Common_IoDevctl,
	.IoUnk21 = (void *)Common_IoUnk21,
};

static PspIoDrv flashfat_driver = //0x4BD4
{
	"flashfat", 0x001E0010, 0x1, "FAT over Flash", &flashfat_funcs
};

STMOD_HANDLER previous; //0x4C9C

char cur_filepath[0xC0]; //0x4CA0

typedef struct
{
	int opened; //0x00
	SceUID fd; //0x04
	int unk_8; //0x08
	char path[0xC0]; //0x0C
	SceMode mode; //0xD0
	int flags; //0xD4
	SceOff offset; //0xD8
} FileHandler; //0xE0

FileHandler file_handler[MAX_FILES];

SceUID sema_id; //0x69B0

//0x00000050
void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}

//0x0000097C
void WriteFile(char *path, void *buf, int size)
{
	SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	sceIoWrite(fd, buf, size);
	sceIoClose(fd);
}

//0x00000764
int SysEventHandler(int ev_id, char *ev_name, void *param, int *result)
{
	if(ev_id == 0x4000) //suspend
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
	else if(ev_id == 0x10009) //resume
	{
		ms_unavailable = 1;
	}

	return 0;
}

//0x00000D68
void WaitMsAvailable()
{
	if(ms_unavailable)
	{
		SceUID fd;
		while((fd = sceIoOpen("ms0:/TM/DC8/ipl.bin", PSP_O_RDONLY, 0)) < 0)
		{
			sceKernelDelayThread(20000);
		}

		sceIoClose(fd);
		ms_unavailable = 0;
	}
}

//0x00000DD0
int WaitFileAvailable(int index, char *path, int flags, SceMode mode)
{
	WaitMsAvailable();

	SceUID fd;

	while(1)
	{
		if(flags == DIR_FLAG) fd = sceIoDopen(path);
		else fd = sceIoOpen(path, flags, mode);

		if(fd != 0x80010018) break;

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

		if(file_handler[index].path != path) strncpy(file_handler[index].path, path, sizeof(file_handler[index].path));
	}

	return fd;
}

//0x00000FC4
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
			
			goto RETURN;
		}

		return fd;
	}

RETURN:
	return file_handler[index].fd;
}

//0x000015DC
void BuildFilePath(char *filename)
{
	strcpy(cur_filepath, "ms0:/TM/DC8");
	strcat(cur_filepath, filename);
}

//0x00001A7C
int _Common_IoOpen(u32 *arg)
{
	PspIoDrvFileArg *drv_arg = (PspIoDrvFileArg *)arg[0];
	char *filename = (char *)arg[1];
	int flags = (int)arg[2];
	SceMode mode = (SceMode)arg[3];

	sceKernelWaitSema(sema_id, 1, NULL);

	BuildFilePath(filename);

	int i;
	for(i = 0; i < MAX_FILES; i++)
	{
		if(file_handler[i].opened == 0)
		{
			SceUID fd = WaitFileAvailable(i, cur_filepath, flags, mode);
			if(fd >= 0)
			{
				drv_arg->arg = (void *)i;
				sceKernelSignalSema(sema_id, 1);
				return 0;
			}

			sceKernelSignalSema(sema_id, 1);
			return fd;
		}
	}

	sceKernelSignalSema(sema_id, 1);
	return 0x80010018;
}

//0x0000142C
int _Common_IoClose(PspIoDrvFileArg *file_arg)
{
	int index = (int)file_arg->arg;

	SceUID fd = GetFileIdByIndex(index);
	if(fd < 0) return fd;
	
	int ret = sceIoClose(fd);
	if(ret < 0) return ret;

	file_handler[index].opened = 0;

	return 0;
}

//0x00001374
int _Common_IoRead(u32 *arg)
{
	PspIoDrvFileArg *file_arg = (PspIoDrvFileArg *)arg[0];
	char *data = (char *)arg[1];
	int len = arg[2];

	sceKernelWaitSema(sema_id, 1, NULL);

	SceUID fd = GetFileIdByIndex((int)file_arg->arg);
	if(fd >= 0)
	{
		int read = sceIoRead(fd, data, len);
		sceKernelSignalSema(sema_id, 1);
		return read;
	}

	sceKernelSignalSema(sema_id, 1);
	return fd;
}

//0x000012A8
int _Common_IoWrite(u32 *arg)
{
	PspIoDrvFileArg *file_arg = (PspIoDrvFileArg *)arg[0];
	int len = arg[2];
	char *data = (char *)arg[1];

	sceKernelWaitSema(sema_id, 1, NULL);

	SceUID fd = GetFileIdByIndex((int)file_arg->arg);
	if(fd >= 0)
	{
		if(!data && len == 0)
		{
			sceKernelSignalSema(sema_id, 1);
			return 0;
		}

		int written = sceIoWrite(fd, data, len);
		sceKernelSignalSema(sema_id, 1);
		return written;
	}

	sceKernelSignalSema(sema_id, 1);
	return fd;
}

//0x00001218
SceOff _Common_IoLseek(u32 *arg)
{
	PspIoDrvFileArg *file_arg = (PspIoDrvFileArg *)arg[0];
	SceOff ofs = (SceOff)arg[1];
	int whence = (int)arg[2];

	sceKernelWaitSema(sema_id, 1, NULL);

	SceUID fd = GetFileIdByIndex((int)file_arg->arg);
	if(fd >= 0)
	{
		int ret = sceIoLseek(fd, ofs, whence);
		sceKernelSignalSema(sema_id, 1);
		return ret;
	}

	sceKernelSignalSema(sema_id, 1);
	return fd;
}

//0x00001A10
int _Common_IoRemove(u32 *arg)
{
	char *name = (char *)arg[1];

	sceKernelWaitSema(sema_id, 1, NULL);

	BuildFilePath(name);
	WaitMsAvailable();

	int ret = sceIoRemove(cur_filepath);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x00001990
int _Common_IoMkdir(u32 *arg)
{
	char *name = (char *)arg[1];
	SceMode mode = (SceMode)arg[2];

	sceKernelWaitSema(sema_id, 1, NULL);

	BuildFilePath(name);
	WaitMsAvailable();

	int ret = sceIoMkdir(cur_filepath, mode);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x00001924
int _Common_IoRmdir(u32 *arg)
{
	char *name = (char *)arg[1];
	
	sceKernelWaitSema(sema_id, 1, NULL);
	
	BuildFilePath(name);
	WaitMsAvailable();
	
	int ret = sceIoRmdir(cur_filepath);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x00001808
int _Common_IoDopen(u32 *arg)
{
	PspIoDrvFileArg *drv_arg = (PspIoDrvFileArg *)arg[0];
	char *dirname = (char *)arg[1];
	
	sceKernelWaitSema(sema_id, 1, NULL);

	BuildFilePath(dirname);

	int i;
	for(i = 0; i < MAX_FILES; i++)
	{		
		if(file_handler[i].opened == 0)
		{
			SceUID fd = WaitFileAvailable(i, cur_filepath, DIR_FLAG, 0);
			if(fd >= 0)
			{
				drv_arg->arg = (void *)i;
				sceKernelSignalSema(sema_id, 1);
				return 0;
			}

			sceKernelSignalSema(sema_id, 1);
			return fd;
		}
	}

	sceKernelSignalSema(sema_id, 1);
	return 0x80010018;
}

//0x00001160
int _Common_IoDclose(PspIoDrvFileArg *file_arg)
{
	sceKernelWaitSema(sema_id, 1, NULL);

	int index = (int)file_arg->arg;

	SceUID fd = GetFileIdByIndex(index);
	if(fd >= 0)
	{
		int ret = sceIoDclose(fd);
		file_handler[index].opened = 0;
		sceKernelSignalSema(sema_id, 1);
		return ret;
	}

	sceKernelSignalSema(sema_id, 1);
	return fd;
}

//0x000010BC
int _Common_IoDread(u32 *arg)
{
	PspIoDrvFileArg *file_arg = (PspIoDrvFileArg *)arg[0];
	SceIoDirent *dir = (SceIoDirent *)arg[1];

	sceKernelWaitSema(sema_id, 1, NULL);

	SceUID fd = GetFileIdByIndex((int)file_arg->arg);
	if(fd >= 0)
	{
		int ret = sceIoDread(fd, dir);
		sceKernelSignalSema(sema_id, 1);
		return ret;
	}

	sceKernelSignalSema(sema_id, 1);
	return fd;
}

//0x00001788
int _Common_IoGetstat(u32 *arg)
{
	char *file = (char *)arg[1];
	SceIoStat *stat = (SceIoStat *)arg[2];

	sceKernelWaitSema(sema_id, 1, NULL);
	
	BuildFilePath(file);
	WaitMsAvailable();
	
	int ret = sceIoGetstat(cur_filepath, stat);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x000016F8
int _Common_IoChstat(u32 *arg)
{
	char *file = (char *)arg[1];
	SceIoStat *stat = (SceIoStat *)arg[2];
	int bits = (int)arg[3];
	
	sceKernelWaitSema(sema_id, 1, NULL);
	
	BuildFilePath(file);
	WaitMsAvailable();
	
	int ret = sceIoChstat(cur_filepath, stat, bits);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x0000167C
int _Common_IoRename(u32 *arg)
{
	char *oldname = (char *)arg[1];
	char *newname = (char *)arg[2];
	
	sceKernelWaitSema(sema_id, 1, NULL);
	
	BuildFilePath(oldname);
	WaitMsAvailable();
	
	int ret = sceIoRename(oldname, newname);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x00001610
int _Common_IoChdir(u32 *arg)
{
	char *dir = (char *)arg[1];

	sceKernelWaitSema(sema_id, 1, NULL);

	BuildFilePath(dir);
	WaitMsAvailable();

	int ret = sceIoChdir(cur_filepath);
	sceKernelSignalSema(sema_id, 1);
	return ret;
}

//0x00000B68
int Common_IoInit(PspIoDrvArg *arg)
{
	sema_id = sceKernelCreateSema("FlashSema", 0, 1, 1, NULL);
	memset(file_handler, 0, sizeof(file_handler));
	
	return 0;
}

//0x00000268
int Common_IoExit(PspIoDrvArg *arg)
{
	return 0;
}

//0x00000640
int Common_IoOpen(PspIoDrvFileArg *arg, char *filename, int flags, SceMode mode)
{
	u32 args[4];
	args[0] = (u32)arg;
	args[1] = (u32)filename;
	args[2] = (u32)flags;
	args[3] = (u32)mode;

	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoOpen, args);
}

//0x00000700
int Common_IoClose(PspIoDrvFileArg *arg)
{
	sceKernelWaitSema(sema_id, 1, NULL);
	int ret = sceKernelExtendKernelStack(0x4000, (void *)_Common_IoClose, arg);
	sceKernelSignalSema(sema_id, 1);

	return ret;
}

//0x0000060C
int Common_IoRead(PspIoDrvFileArg *arg, char *data, int len)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)data;
	args[2] = (u32)len;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoRead, args);
}

//0x000005D8
int Common_IoWrite(PspIoDrvFileArg *arg, const char *data, int len)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)data;
	args[2] = (u32)len;

	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoWrite, args);
}

//0x000005A0
SceOff Common_IoLseek(PspIoDrvFileArg *arg, SceOff ofs, int whence)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)ofs;
	args[2] = (u32)whence;

	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoLseek, args);
}

//0x00000A48
int Common_IoIoctl(PspIoDrvFileArg *arg, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	sceKernelWaitSema(sema_id, 1, NULL);

	if(cmd < 0x00208008)
	{
		if(cmd < 0x00208006)
		{
			if(cmd != 0x00008003)
			{
				if(cmd < 0x00008004)
				{
					if(cmd != 0x00005001) goto UNKNOWN;
				}
				else
				{
					if(cmd != 0x0000B804)
					{
						if(cmd != 0x00208003) goto UNKNOWN;
					}
				}
			}
		}
	}
	else if(cmd != 0x00208081)
	{
		if(cmd == 0x00208082)
		{
			sceKernelSignalSema(sema_id, 1);
			return 0x80010016;
		}
		else
		{
			if(cmd != 0x00208013)
			{
UNKNOWN:
				WriteFile("fatms0:/unk_ioctl.bin", &cmd, sizeof(u32));
				while(1);
			}

			if(arg->fs_num != 3 || sceKernelGetModel() == 0)
			{
				sceKernelSignalSema(sema_id, 1);
				return 0x80010016;
			}
		}
	}

	sceKernelSignalSema(sema_id, 1);
	return 0;
}

//0x00000570
int Common_IoRemove(PspIoDrvFileArg *arg, const char *name)
{
	u32 args[2];
	args[0] = (u32)arg;
	args[1] = (u32)name;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoRemove, args);
}

//0x0000053C
int Common_IoMkdir(PspIoDrvFileArg *arg, const char *name, SceMode mode)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)name;
	args[2] = (u32)mode;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoMkdir, args);
}

//0x0000050C
int Common_IoRmdir(PspIoDrvFileArg *arg, const char *name)
{
	u32 args[2];
	args[0] = (u32)arg;
	args[1] = (u32)name;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoRmdir, args);
}

//0x000004DC
int Common_IoDopen(PspIoDrvFileArg *arg, const char *dirname)
{
	u32 args[2];
	args[0] = (u32)arg;
	args[1] = (u32)dirname;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoDopen, args);
}

//0x000004C8
int Common_IoDclose(PspIoDrvFileArg *arg)
{
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoDclose, arg);
}

//0x00000498
int Common_IoDread(PspIoDrvFileArg *arg, SceIoDirent *dir)
{
	u32 args[2];
	args[0] = (u32)arg;
	args[1] = (u32)dir;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoDread, args);
}

//0x00000464
int Common_IoGetstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)file;
	args[2] = (u32)stat;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoGetstat, args);
}

//0x0000042C
int Common_IoChstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat, int bits)
{
	u32 args[4];
	args[0] = (u32)arg;
	args[1] = (u32)file;
	args[2] = (u32)stat;
	args[3] = (u32)bits;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoChstat, args);
}

//0x000003F8
int Common_IoRename(PspIoDrvFileArg *arg, const char *oldname, const char *newname)
{
	u32 args[3];
	args[0] = (u32)arg;
	args[1] = (u32)oldname;
	args[2] = (u32)newname;
	
	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoRename, args);
}

//0x000003C8
int Common_IoChdir(PspIoDrvFileArg *arg, const char *dir)
{
	u32 args[2];
	args[0] = (u32)arg;
	args[1] = (u32)dir;

	return sceKernelExtendKernelStack(0x4000, (void *)_Common_IoChdir, args);
}

//0x00000270
int Common_IoMount(PspIoDrvFileArg *arg)
{
	return 0;
}

//0x00000278
int Common_IoUmount(PspIoDrvFileArg *arg)
{
	return 0;
}

//0x000009D4
int Common_IoDevctl(PspIoDrvFileArg *arg, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
	if(cmd == 0x00005802)
	{
		return 0;
	}
	else if(cmd == 0x00208813)
	{
		if(arg->fs_num != 3 || sceKernelGetModel() == 0) return 0x80010016;
	}
	else
	{
		WriteFile("fatms0:/unk_devctl.bin", &cmd, sizeof(u32));
		while(1);
	}

	return 0;
}

//0x00000280
int Common_IoUnk21(PspIoDrvFileArg *arg)
{
	return 0x80010086;
}

//0x0000028C
int dummy_ok()
{
	return 0;
}

//0x00000294
int dummy_error()
{
	return 0x80010086;
}

//0x000002A0
int sceLfatfsStop()
{
	return 0;
}

//0x000002A8
int sceLFatFs_driver_51C7F7AE()
{
	return 0x05000010;
}

//0x000002B4
int sceLFatFs_driver_F1FBA85F()
{
	return 0;
}

//0x00000BB8
int sceLfatfsWaitReady()
{
	if(thid >= 0) sceKernelWaitThreadEnd(thid, 0);
	return 0;
}

//0x000014A4
int SceLfatfsAssign(SceSize args, void *argp)
{
	WaitMsAvailable();

	sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", IOASSIGN_RDONLY, NULL, 0);
	sceIoAssign("flash1:", "lflash0:0,1", "flashfat1:", IOASSIGN_RDWR, NULL, 0);

	if(sceKernelInitKeyConfig() == PSP_INIT_KEYCONFIG_VSH) sceIoAssign("flash2:", "lflash0:0,2", "flashfat2:", IOASSIGN_RDWR, NULL, 0);

	if(sceKernelGetModel() == 1)
	{
		int type = sceKernelInitKeyConfig();
		if(type == PSP_INIT_KEYCONFIG_VSH || (type == 0x400 && sceKernelBootFrom() == 0x80))
		{
			sceIoAssign("flash3:", "lflash0:0,3", "flashfat3:", IOASSIGN_RDWR, NULL, 0);
		}
	}

	thid = -1;
	sceKernelExitDeleteThread(0);
	return 0;
}

//0x00000330
void FlashEmuInit()
{	
	sceIoDelDrv("lflash");
	sceIoAddDrv(&lflash_driver);

	sceIoDelDrv("flashfat");
	sceIoAddDrv(&flashfat_driver);

	sceKernelRegisterSysEventHandler(&event_handler);

	thid = sceKernelCreateThread("SceLfatfsAssign", SceLfatfsAssign, 0x64, 0x1000, 0x100000, NULL);
	if(thid >= 0) sceKernelStartThread(thid, 0, NULL);
}

//0x000002BC
int FlashEmuExit()
{
	return 0;
}

//0x00000854
int sceKernelExtendKernelStack_Patch_Callback(u32 *arg)
{
	sceKernelWaitSema(sema_id, 1, NULL);

	int i;
	for(i = 0; i < MAX_FILES; i++)
	{
		if(file_handler[i].opened && file_handler[i].unk_8 == 0 && file_handler[i].flags == PSP_O_RDONLY)
		{
			file_handler[i].offset = sceIoLseek(file_handler[i].fd, 0, PSP_SEEK_CUR);
			sceIoClose(file_handler[i].fd);

			file_handler[i].unk_8 = 1;

			sceKernelSignalSema(sema_id, 1);
			return 0;
		}
	}

	sceKernelSignalSema(sema_id, 1);
	return 0x80010018;
}

//0x00000CA0
int sceKernelExtendKernelStackPatched1(int type, void (* cb)(void *), u32 *arg)
{
	int ret;

	do
	{
		ret = sceKernelExtendKernelStack(type, cb, arg);
		if(ret != 0x80010018 || arg[1] == 0 || memcmp((void *)(arg[1] + 4), buf_4B6C, sizeof(buf_4B6C)) == 0) break;
		ret = sceKernelExtendKernelStack(0x4000, (void *)sceKernelExtendKernelStack_Patch_Callback, NULL);
	} while(ret >= 0);

	return ret;
}

//0x00000BD8
int sceKernelExtendKernelStackPatched2(int type, void (* cb)(void *), u32 *arg)
{
	int ret;

	do
	{
		ret = sceKernelExtendKernelStack(type, cb, arg);
		if(ret != 0x80010018 || arg[1] == 0 || memcmp((void *)(arg[1] + 4), buf_4B6C, sizeof(buf_4B6C)) == 0) break;
		ret = sceKernelExtendKernelStack(0x4000, (void *)sceKernelExtendKernelStack_Patch_Callback, NULL);
	} while(ret >= 0);

	return ret;
}

//0x00000678
int sceKernelExtendKernelStackPatched3(int type, void (* cb)(void *), void *arg)
{
	int ret;

	do
	{
		ret = sceKernelExtendKernelStack(type, cb, arg);
		if(ret != 0x80010018) break;
		ret = sceKernelExtendKernelStack(0x4000, (void *)sceKernelExtendKernelStack_Patch_Callback, NULL);
	} while(ret >= 0);
	
	return ret;
}

//0x00000000
int sceKernelGzipDecompressPatched(u8 *dest, int destSize, u8 *src, u32 unknown)
{
	if(sceKernelGetModel() == 0) src = rebootex_01g;
	else src = rebootex_02g;

	return sceKernelGzipDecompress(dest, destSize, src, 0);
}

//0x0000006C
void PatchSystemControl()
{
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("SystemControl");

	if(sceKernelGetModel() == 0)
	{
		MAKE_CALL(mod->text_addr + 0x2408, sceKernelGzipDecompressPatched);
	}
	else
	{
		MAKE_CALL(mod->text_addr + 0x2690, sceKernelGzipDecompressPatched);
	}
	
	ClearCaches();
}

//0x00000120
int OnModuleStart(SceModule2 *mod)
{
	char *modname = mod->modname;

	if(strcmp(modname, "sceMediaSync") == 0)
	{
		SceModule2 *msfat_drv = (SceModule2 *)sceKernelFindModuleByName("sceMSFAT_Driver");

		MAKE_CALL(msfat_drv->text_addr + 0x48D4, sceKernelExtendKernelStackPatched1);
		MAKE_CALL(msfat_drv->text_addr + 0x5338, sceKernelExtendKernelStackPatched2);
		MAKE_CALL(msfat_drv->text_addr + 0x5B90, sceKernelExtendKernelStackPatched3);

		ClearCaches();
	}
	else if(strcmp(modname, "sceLflashFatfmt") == 0)
	{
		u32 sceLflashFatfmtStartFatfmt = FindProc("sceLflashFatfmt", "LflashFatfmt", 0xB7A424A4);
		if(sceLflashFatfmtStartFatfmt)
		{
			MAKE_DUMMY_FUNCTION0(sceLflashFatfmtStartFatfmt);
			ClearCaches();
		}
	}

	if(!previous)
		return 0;

	return previous(mod);
}

//0x000000E0
int module_start(SceSize args, void *argp)
{
	PatchSystemControl();
	FlashEmuInit();

	previous = sctrlHENSetStartModuleHandler(OnModuleStart);

	ClearCaches();

	return 0;
}

//0x000002C4
int module_reboot_before(SceSize args, void *argp)
{
	SceUInt timeout = 500000;
	sceKernelWaitSema(sema_id, 1, &timeout);
	sceKernelDeleteSema(sema_id);

	sceIoUnassign("flash0:");
	sceIoUnassign("flash1:");

	sceKernelUnregisterSysEventHandler(&event_handler);

	return 0;
}