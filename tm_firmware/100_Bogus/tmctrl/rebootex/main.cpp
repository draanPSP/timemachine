#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>
#include <syscon.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using v_iii_function_t = std::add_pointer_t<void(s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*)>;

	inline  v_iii_function_t rebootEntryPtr;

	char path[260];

	[[noreturn]] inline void rebootEntry(s32 const a0, s32 const a1, s32 a2) {
		rebootEntryPtr(a0, a1, a2);

		__builtin_unreachable();
	}

	void clearCaches() {
		iplKernelDcacheWritebackInvalidateAll();
		iplKernelIcacheInvalidateAll();
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

		strcpy(path, TM_PATH);

		if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcat(path, "/tmctrl100_Bogus.prx");
		}
		else
			strcat(path, filename);

		if (f_open(&fp, path, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
			return 1;
		}

		return -1;
	}

	int sceBootLfatReadPatched(void *buffer, int buffer_size) {

		u32 bytes_read;
		if (f_read(&fp, buffer, buffer_size, &bytes_read) == FR_OK)
			return bytes_read;

		return 0;
	}

	int sceBootLfatClosePatched() {

		f_close(&fp);
		return 0;
	}

	int sysMemModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start) {

		return module_start(4, argp);
	}

	int loadCoreIModuleStartPatched(module_start_function_t module_start, SceSize args, void *argp) {

		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0x0cf4;

		// NoPlainModuleCheckPatch (mfc0 $v0, $22 -> li $v0, 1)
		_sw(0x24020001, text_addr + 0x4160);

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2,u32 reboot_text_addr) {

	rebootEntryPtr = reinterpret_cast<v_iii_function_t>(reboot_text_addr + 0x170);

	// jal sysmem::module_start -> jal SysMemStart
	// li  $a0, 4 -> mov $a2, $v0  (a0 = 4 -> a2 = module_start)
	MAKE_CALL(reboot_text_addr + 0xff4, sysMemModuleStartPatched);
	_sw(0x00403021, reboot_text_addr + 0xff8);
	
	MAKE_JUMP(reboot_text_addr + 0xfdc, loadCoreIModuleStartPatched);

	/* File open redirection */
	MAKE_CALL(reboot_text_addr + 0x08, sceBootLfatMountPatched);
	MAKE_CALL(reboot_text_addr + 0x9c, sceBootLfatOpenPatched);
	MAKE_CALL(reboot_text_addr + 0xcc, sceBootLfatReadPatched);
	MAKE_CALL(reboot_text_addr + 0xf8, sceBootLfatClosePatched);

	clearCaches();

	iplSysregGpioIoEnable(GpioPort::SYSCON_REQUEST);
	iplSysregGpioIoEnable(GpioPort::MS_LED);
	iplSysregGpioIoEnable(GpioPort::WLAN_LED);

	sdkSync();
	
	iplSysconInit();
	iplSysconCtrlMsPower(true);
	
	rebootEntry(a0, a1, a2);
}