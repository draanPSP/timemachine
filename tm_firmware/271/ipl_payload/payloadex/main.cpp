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

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x8841bef0);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x8841bb20);
	inline auto const payloadEntryPtr = reinterpret_cast<v_iiii_function_t const>(0x88400000);

	char path[260];
	bool clockgenLoaded = false;

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

		if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl150.prx");
		}
		else if (strcmp(filename, "/kn/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl271.prx");
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

		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0x0ab8;

		// NoPlainModuleCheckPatch (mfc0 $t5, $22 -> li $t5, 1)
		_sw(0x340d0001, text_addr + 0x2fe0);

		/* NoPlainPspConfigCheckPatch */
	
		// mfc0 $a1, $22 -> nop
		_sw(0, text_addr+0x284C);
		// li $a0, 0xFFFFFFFF -> mov $a0, $a1 -> (a0 = -1 -> a0 = filesize)
		_sw(0x00052021, text_addr+0x285c);

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

	// jal sysmem::module_start -> jal SysMemStart
	// li  $a0, 4 -> mov $a2, $s1  (a0 = 4 -> a2 = module_start)
	MAKE_CALL(0x884007e4, sysMemModuleStartPatched);
	_sw(0x02203021, 0x884007e8);
	
	// mov $a0, $s4 -> mov $a2, $v0 (a0 = 8 -> a2 = module_start)
	// mov $a1, $sp
	// jr  loadcore::module_start -> j LoadCoreStart 
	// mov $sp, $t3
	_sw(0x00403021, 0x884007c0);
	MAKE_JUMP(0x884007c8, loadCoreModuleStartPatched);

	/* NoPlainPspConfigCheckPatch */
	
	// mfc0 $a1, $22 -> nop
	_sw(0, 0x8840d81c);
	// li   a0, 0xFFFFFFFF -> mov $a0, $a1 (a0 = -1 -> a0 = filesize)
	_sw(0x00052021, 0x8840d82c);
	// li   a3, 0xFFFFFFFF -> mov $a3, $a1 (return -1 -> return filesize)
	_sw(0x00053821, 0x8840d834);

	// NoPlainModuleCheckPatch (mfc0 t6, $22 -> li $t6, 1)
	_sw(0x240e0001, 0x8840df9c);

	// Patch removeByDebugSection, make it return 1	
	_sw(0x03e00008, 0x884130b0);
	_sw(0x24020001, 0x884130b4);

	/* File open redirection */

	// Redirect sceBootLfatMount to MsFatMount
	MAKE_CALL(0x8840111c, sceBootLfatMountPatched);
	// Redirect sceBootLfatOpen to MsFatOpen
	MAKE_CALL(0x8840112c, sceBootLfatOpenPatched);
	// Redirect sceBootLfatRead to MsFatRead
	MAKE_CALL(0x8840115c, sceBootLfatReadPatched);
	// Redirect sceBootLfatClose to MsFatClose
	MAKE_CALL(0x8840117c, sceBootLfatClosePatched);

	clearCaches();
	
	payloadEntry(a0, a1, a2, a3);
}