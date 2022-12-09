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
	using v_iiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*, void*)>;

	inline auto const cache1Ptr150 = reinterpret_cast<v_v_function_t const>(0x88C03F50);
	inline auto const cache2Ptr150 = reinterpret_cast<v_v_function_t const>(0x88C03B80);
	inline auto const cache1Ptr340 = reinterpret_cast<v_v_function_t const>(0x886027C0);
	inline auto const cache2Ptr340 = reinterpret_cast<v_v_function_t const>(0x8860218C);
	inline auto const oeRebootexEntryPtr = reinterpret_cast<v_iiii_function_t const>(0x88FC0000);

	char path[260];

	void clearCaches150() {
		cache1Ptr150();
		cache2Ptr150();
	}

	void clearCaches340() {
		cache1Ptr340();
		cache2Ptr340();
	}

	[[noreturn]] inline void oeRebootexEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

		oeRebootexEntryPtr(a0, a1, a2, a3);

		__builtin_unreachable();
	}

	int patchLoadCore340(SceSize a0, void *a1, void *a2, module_start_function_t module_start) {

		_sw(0x1021, (u32)module_start + 0x5518); // jal memlmd_323366CA -> move $v0, $zr
		_sw(0x1021, (u32)module_start + 0x5548); // jal memlmd_323366CA -> move $v0, $zr
		_sw(0x1021, (u32)module_start + 0x55ac); // jal memlmd_323366CA -> move $v0, $zr

		clearCaches340();

		return module_start(a0, a1, a2);
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

		if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl150.prx");
		}
		else if (strcmp(filename, "/kn/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl340.prx");
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
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

	if (strncmp((char*)0x88c16cb2, "19196", 5) == 0)
	{
		// 150 reboot

		// Patch removeByDebugSection, make it return 1	
		_sw(0x03e00008, 0x88C01D20);
		_sw(0x24020001, 0x88C01D24);

		/* File open redirection */
		MAKE_CALL(0x88c00074, sceBootLfatMountPatched);
		MAKE_CALL(0x88C00084, sceBootLfatOpenPatched);
		MAKE_CALL(0x88C000B4, sceBootLfatReadPatched);
		MAKE_CALL(0x88C000E0, sceBootLfatClosePatched);
		clearCaches150();
	}
	else
	{
		// 340 reboot

		_sw(0x03e00008, 0x886037c0);
		_sw(0x1021, 0x886037c4);

		MAKE_CALL(0x88fc06c0, patchLoadCore340);
		_sw(0x403821, 0x88fc06c4); //move $a3, $v0 -> a3 = module_start

		/* File open redirection */
		MAKE_CALL(0x88600080, sceBootLfatMountPatched);
		_sw((u32)sceBootLfatOpenPatched, 0x88fc0a24);
		MAKE_CALL(0x886000c0, sceBootLfatReadPatched);
		MAKE_CALL(0x886000ec, sceBootLfatClosePatched);

		clearCaches340();
	}

	iplSysregGpioIoEnable(GpioPort::SYSCON_REQUEST);
	iplSysregGpioIoEnable(GpioPort::MS_LED);
	iplSysregGpioIoEnable(GpioPort::WLAN_LED);

	sdkSync();
	
	iplSysconInit();
	iplSysconCtrlMsPower(true);
	
	oeRebootexEntry(a0, a1, a2, a3);
}