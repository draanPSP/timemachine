#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>
#include <syscon.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>
#include <plainModulesPatch.h>

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using v_iiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*)>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x88c03244);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x88c02c10);
	inline auto const rebootEntryPtr = reinterpret_cast<v_iiii_function_t const>(0x88c00000);

	char path[260];

	FATFS fs;
	FIL fp;

	[[noreturn]] inline void rebootEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {
		
		rebootEntryPtr(a0, a1, a2, a3);

		__builtin_unreachable();
	}

	void clearCaches() {
		cache1Ptr();
		cache2Ptr();
	}

	int sceBootLfatMountPatched() {

		if (f_mount(&fs, "ms0:", 1) != FR_OK) {
			return -1;
		}

		return 0;
	}

	int sceBootLfatOpenPatched(char *filename) {

		if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl280.prx");
		}

		strcpy(path, TM_PATH);
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

	int loadCoreModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start) {

		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0x0bb8;

		KERNEL_HIJACK_FUNCTION(text_addr + 0x36e4, sceKernelCheckExecFileHook, _sceKernelCheckExecFile, (int (*)(u8*, LoadCoreExecInfo*)));

		// Fix for error "unacceptable relocation type: 0x7", restore 2.00 behaviour
		_sw(_lw(text_addr + 0x72e4), text_addr + 0x72e4 + 7 * sizeof(u32));

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

	// jal sysmem::module_start -> jal SysMemStart
	// li  $a0, 4 -> mov $a2, $s2  (a0 = 4 -> a2 = module_start)
	MAKE_CALL(0x88c050c4, sysMemModuleStartPatched);
	_sw(0x02403021, 0x88c050c8);
	
	_sw(0x02203021, 0x88c050a0);
	MAKE_JUMP(0x88c050a8, loadCoreModuleStartPatched);

	// Patch removeByDebugSection, make it return 0
	MAKE_FUNCTION_RETURN0(0x88c00fcc);

	// NoPlainPspConfigCheckPatch
	_sw(0xafa50000, 0x88c0543c);
	_sw(0x20a30000, 0x88c05440);

	// Disable sign check descramble of boot configs, sysmem and loadcore
	MAKE_FUNCTION_RETURN0(0x88c04198);

	// Disable module hash check
	_sw(0x00001021, 0x88c048c8);

	// disable setting of sign checked module attr flag on load core module info
	_sw(0, 0x88c04858);
	_sw(0, 0x88c048e0);

	/* File open redirection */
	MAKE_CALL(0x88c00078, sceBootLfatMountPatched);
	MAKE_CALL(0x88c0008c, sceBootLfatOpenPatched);
	MAKE_CALL(0x88c000b8, sceBootLfatReadPatched);
	MAKE_CALL(0x88c000e4, sceBootLfatClosePatched);

	clearCaches();

	iplSysregGpioIoEnable(GpioPort::SYSCON_REQUEST);
	iplSysregGpioIoEnable(GpioPort::MS_LED);
	iplSysregGpioIoEnable(GpioPort::WLAN_LED);

	sdkSync();
	
	iplSysconInit();
	iplSysconCtrlMsPower(true);
	
	rebootEntry(a0, a1, a2, a3);
}