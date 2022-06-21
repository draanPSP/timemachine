#include <string.h>

#include <pspkernel.h>
#include <pspsdk.h>
#include <pspsysevent.h>
#include <psputilsforkernel.h>

#include "flashemu.h"

PSP_MODULE_INFO("Flashemu", PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_START | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_NO_STOP, 1, 0);

int SysEventHandler(int eventId, char *eventName, void *param, int *result);

extern SceUID flashemu_sema;
extern int msNotReady;

PspSysEventHandler sysEventHandler =
	{
		.size = sizeof(PspSysEventHandler),
		.name = "",
		.type_mask = 0x0000FF00,
		.handler = SysEventHandler};


int SysEventHandler(int eventId, char *eventName, void *param, int *result)
{
	if (eventId == 0x10009) //resume
		msNotReady = 1;

	return 0;
}

int module_start(SceSize args, void *argp)
{
	InstallFlashEmu();

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