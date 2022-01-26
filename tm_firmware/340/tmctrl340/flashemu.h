#ifndef __FLASHEMU_H__
#define __FLASHEMU_H__

int InstallFlashEmu();
int UninstallFlashEmu();

int df_openPatched(s32 a0, char* path, s32 a2, s32 a3);
int df_dopenPatched(s32 a0, char* path, s32 a2);
int df_devctlPatched(s32 a0, s32 a1, s32 a2, s32 a3);

#define MAX_FILES 32
#define DIR_FLAG 0xd0d0

typedef struct
{
	int opened;
	SceUID fd;
	int unk_8;
	char path[0xC0];
	SceMode mode;
	int flags;
	SceOff offset;
} FileHandler;

#endif