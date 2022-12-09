#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using v_iiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*)>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x88419f00);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x88419b30);
	inline auto const payloadEntryPtr = reinterpret_cast<v_iiii_function_t const>(0x88400000);

	char path[260];

	[[noreturn]] inline void payloadEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {
		payloadEntryPtr(a0, a1, a2, a3);

		__builtin_unreachable();
	}

	void clearCaches() {
		cache1Ptr();
		cache2Ptr();
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

		if(strcmp(filename, "/kd/lfatfs.prx") == 0)
			strcat(path, "/tmctrl100_Bogus.prx");
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

	int loadCoreIModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start) {

		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0x0cf4;

		// NoPlainModuleCheckPatch (mfc0 $v0, $22 -> li $v0, 1)
		_sw(0x24020001, text_addr + 0x4160);

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

	// jal sysmem::module_start -> jal SysMemStart
	// li  $a0, 4 -> mov $a2, $v0  (a0 = 4 -> a2 = module_start)
	MAKE_CALL(0x88400d68, sysMemModuleStartPatched);
	_sw(0x00403021, 0x88400d6C);
	
	// mov  $a0, $a1 -> mov $a1, $a2
	// mov  $a1, $a2 -> mov $a2, $v0
	// jr   loadcore::module_start -> j LoadCoreStart 
	// mov  $sp, $a3
	_sw(0x00c02821, 0x88400bbc);
	_sw(0x00403021, 0x88400bc0);
	MAKE_JUMP(0x88400bc4, loadCoreIModuleStartPatched);

	/* NoPlainPspConfigCheckPatch */
	
	// mfc0 $v0, $22 -> li $v0, 1
	_sw(0x24020001, 0x8840d740);
	// li   $v0, 0xFFFFFFFF -> mov $v0, $a1 (v0 = -1 -> v0 = filesize)
	_sw(0x00051021, 0x8840d82c);
	// li   a3, 0xFFFFFFFF -> mov $a3, $a1 (return -1 -> return filesize)
	_sw(0x00053821, 0x8840d758);

	// NoPlainModuleCheckPatch (mfc0 v0, $22 -> li $v0, 1)
	_sw(0x24020001, 0x8840e160);

	// Patch removeByDebugSection, make it return 1	
	_sw(0x03e00008, 0x88413114);
	_sw(0x24020001, 0x88413118);

	/* File open redirection */
	
	// Redirect sceBootLfatMount to MsFatMount
	MAKE_CALL(0x8840113c, sceBootLfatMountPatched);
	// Redirect sceBootLfatOpen to MsFatOpen
	MAKE_CALL(0x884011cc, sceBootLfatOpenPatched);
	// Redirect sceBootLfatRead to MsFatRead
	MAKE_CALL(0x884011fc, sceBootLfatReadPatched);
	// Redirect sceBootLfatClose to MsFatClose
	MAKE_CALL(0x8840121c, sceBootLfatClosePatched);

	clearCaches();
	
	payloadEntry(a0, a1, a2, a3);
}