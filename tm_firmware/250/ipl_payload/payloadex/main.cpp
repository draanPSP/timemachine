#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>

#include <type_traits>

#include <tm_common.h>
#include <plainModulesPatch.h>
#include <pspmacro.h>

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using v_iiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*)>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x8841d3f4);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x8841cf18);
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

		if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl250.prx");
		}

		strcpy(path, "/TM/250");
		strcat(path, filename);

		if (f_open(&fp, path, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
			return 1;
		}

		return -1;
	}

	int sceBootLfatReadPatched(void *buffer, int buffer_size)
	{
		u32 bytes_read;
		if (f_read(&fp, buffer, buffer_size, &bytes_read) == FR_OK)
			return bytes_read;

		return 0;
	}

	int sceBootLfatClosePatched()
	{
		f_close(&fp);
		return 0;
	}

	int sysMemModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start)
	{
		return module_start(4, argp);
	}

	int loadCoreModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start)
	{
		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0x0bf0;

		KERNEL_HIJACK_FUNCTION(text_addr + 0x352c, sceKernelCheckExecFileHook, _sceKernelCheckExecFile, (int (*)(u8*, LoadCoreExecInfo*)));

		// Fix for error "unacceptable relocation type: 0x7", restore 2.00 behaviour
		_sw(_lw(text_addr + 0x7c34), text_addr + 0x7c34 + 7 * sizeof(u32));

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {

	// jal sysmem::module_start -> jal SysMemStart
	// li  $a0, 4 -> mov $a2, $s2  (a0 = 4 -> a2 = module_start)
	MAKE_CALL(0x8840083c, sysMemModuleStartPatched);
	_sw(0x02403021, 0x88400840);
	
	// mov $a0, $a3 -> mov $a2, $s4 (a0 = 8 -> a2 = module_start)
	// mov $a1, $sp
	// jr  loadcore::module_start -> j LoadCoreStart 
	// mov $sp, $v1
	_sw(0x02803021, 0x884007f0);
	MAKE_JUMP(0x884007f8, loadCoreModuleStartPatched);

	/* NoPlainPspConfigCheckPatch */

	_sw(0xafa50000, 0x8840d370);
	_sw(0x20a20000, 0x8840d374);

	// Patch removeByDebugSection, make it return 1	
	_sw(0x03e00008, 0x88413450);
	_sw(0x24020001, 0x88413454);

	// Disable sign check descramble of psp boot configs, sysmem and loadcore
	_sw(0x3C040000, 0x884004c8);

	// disable setting of sign checked module attr flag on load core module info
	_sw(0, 0x88400abc);

	/* File open redirection */

	MAKE_CALL(0x88401710, sceBootLfatMountPatched);
	MAKE_CALL(0x88401720, sceBootLfatOpenPatched);
	MAKE_CALL(0x8840174c, sceBootLfatReadPatched);
	MAKE_CALL(0x8840176c, sceBootLfatClosePatched);

	clearCaches();
	
	payloadEntry(a0, a1, a2, a3);
}