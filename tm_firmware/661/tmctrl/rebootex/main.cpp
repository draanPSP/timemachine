/* 
 * Based on rebootex/main.c from minimum edition - https://github.com/PSP-Archive/minimum_edition
*/

#include <string.h>
#include <type_traits>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>
#include <syscon.h>

#include <pspmacro.h>
#include <tm_common.h>

#include "rebootPatches.h"
#include "rebootex_config.h"
#include "btcnf.h"

#if PSP_MODEL == 0
#define SYSTEMCTRL_PATH "systemctrl_01g.prx"
#elif PSP_MODEL == 1
#define SYSTEMCTRL_PATH "systemctrl_02g.prx"
#endif

namespace {

	using v_iiiiiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32,s32,s32,s32)>;
	inline auto const rebootEntryPtr = reinterpret_cast<v_iiiiiii_function_t const>(0x88600000);

	int elf_load_flag;
	int btcnf_load_flag;
	int rtm_flag;

	char *systemctrl = (char *)0x88FB0100;
	u32 sizeSystemctrl = 0;

	char *onRebootAfter = NULL;
	void *onRebootBuf = NULL;
	u32 onRebootSize = 0;
	u32 onRebootFlag = 0;

	int bootIndex = 0;

	char path[260];

	[[noreturn]] inline void rebootEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {
		rebootEntryPtr(a0, a1, a2, a3, t0, t1, t2);

		__builtin_unreachable();
	}

	FATFS fs;
	FIL fp;

	int sceBootLfatMountPatched() {

		if (f_mount(&fs, "ms0:", 1) != FR_OK) {
			return -1;
		}

		return 0;
	}

	int sceBootLfatOpenPatched(char *filename) {

		if (memcmp(path + 4, SYSTEMCTRL_PATH, sizeof(SYSTEMCTRL_PATH)) == 0)
		{
			if(sizeSystemctrl)
			{	
				elf_load_flag = 1;	
				return 0;
			}
		}
		else if (memcmp(filename + 4, "pspbtcnf", sizeof("pspbtcnf") - 1) == 0)
		{
			filename[9] = 'j';
			btcnf_load_flag = 1;
		}
		else if (memcmp(filename, "/rtm.prx" , sizeof("/rtm.prx")) == 0)
		{
			rtm_flag = 1;
			return 0;
		}
		else if (strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl661.prx");
		}
		else if (bootIndex == 1)
		{
			if(memcmp(path + 4, "isotope.prx", sizeof("isotope.prx")) == 0)
			{
				memcpy(path + 4 , "dax9660.prx", sizeof("dax9660.prx"));
			}
		}

		strcpy(path, TM_PATH);
		strcat(path, filename);

		if (f_open(&fp, path, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
			return 0;
		}

		return -1;
	}

	int sceBootLfatReadPatched(void *buffer, int buffer_size) {

		if (elf_load_flag)//elf load flag1
		{
			int load_size = sizeSystemctrl;
			if( load_size > buffer_size)		
				load_size = buffer_size;

			memcpy(buffer, systemctrl, sizeSystemctrl);

			sizeSystemctrl -= load_size;
			systemctrl += load_size;

			return load_size;
		}
		else if (rtm_flag)
		{
			int load_size = onRebootSize;

			if( load_size > buffer_size)
				load_size = buffer_size;

			memcpy(buffer, onRebootBuf, load_size);

			onRebootSize -= load_size;
			onRebootBuf += load_size;
			return load_size;
		}

		u32 ret;
		f_read(&fp, buffer, buffer_size, &ret);

		if (btcnf_load_flag)
		{
			switch(bootIndex)
			{
			case 2://np9660
				ret = btcnf_patch(buffer, ret, module_np9660, 0x40, 2);
				break;
			case 1://M33
			case 3://me	
				ret = btcnf_patch(buffer, ret, module_isotope, 0x40, 2);
				break;
			}

			if (bootIndex == 4)
			{
				ret = btcnf_patch(buffer, ret, module_recovery, 0, 0);
			}
			else if (onRebootAfter)
			{
				module_rtm[0].before_path = onRebootAfter;
				module_rtm[0].flag = onRebootFlag;

				ret = btcnf_patch(buffer, ret , module_rtm, 0, 0);
			}

			btcnf_load_flag = 0;
		}

		return ret;
	}

	int sceBootLfatClosePatched() {

		if(elf_load_flag)
		{
			elf_load_flag = 0;
			return 0;
		}
		else if(rtm_flag)
		{
			rtm_flag = 0;
			return 0;
		}

		f_close(&fp);
	
		return 0;
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {

	struct RebootexParam *rebootex_param = reinterpret_cast<RebootexParam *>(REBOOTEX_PARAM_OFFSET);

	sizeSystemctrl = *reinterpret_cast<u32 *>(0x88FB00F0);

	if(sizeSystemctrl == -1)
		sizeSystemctrl = 0;

	bootIndex = rebootex_param->reboot_index;

	onRebootAfter = reinterpret_cast<char *>(rebootex_param->on_reboot_after);
	onRebootBuf	= rebootex_param->on_reboot_buf;
	onRebootSize	= rebootex_param->on_reboot_size;
	onRebootFlag	= rebootex_param->on_reboot_flag;

	elf_load_flag = 0;
	btcnf_load_flag = 0;
	rtm_flag = 0;

	MsLfatFuncs funcs = {
		.msMount = reinterpret_cast<void *>(sceBootLfatMountPatched),
		.msOpen = reinterpret_cast<void *>(sceBootLfatOpenPatched),
		.msRead = reinterpret_cast<void *>(sceBootLfatReadPatched),
		.msClose = reinterpret_cast<void *>(sceBootLfatClosePatched),
	};

	patchRebootBin(&funcs);

	iplSysregGpioIoEnable(GpioPort::SYSCON_REQUEST);
	iplSysregGpioIoEnable(GpioPort::MS_LED);
	iplSysregGpioIoEnable(GpioPort::WLAN_LED);

	sdkSync();
	
	iplSysconInit();
	iplSysconCtrlMsPower(true);

	rebootEntry(a0, a1, a2, a3, t0, t1, t2);
}