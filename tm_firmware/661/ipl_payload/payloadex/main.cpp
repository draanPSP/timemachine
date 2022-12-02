#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>

#include <pspmacro.h>
#include <type_traits>
#include <tm_common.h>

#include "rebootPatches.h"
#include "rebootex_config.h"

#if PSP_MODEL == 0
#define BTCNF_PATH "/kd/pspbtcnf.bin"
#elif PSP_MODEL == 1
#define BTCNF_PATH "/kd/pspbtcnf_02g.bin"
#endif

namespace {
	using v_iiiiiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32,s32,s32,s32)>;
	inline auto const payloadEntryPtr = reinterpret_cast<v_iiiiiii_function_t const>(0x88600000);

	char path[260];

	void clearCaches() {
		iplKernelDcacheWritebackInvalidateAll();
		iplKernelIcacheInvalidateAll();
	}

	[[noreturn]] inline void payloadEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {
		payloadEntryPtr(a0, a1, a2, a3, t0, t1, t2);

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

		if(strcmp(filename, BTCNF_PATH) == 0) {
			filename[9] = 'j';
		}
		else if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl661.prx");
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

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {

	memset(reinterpret_cast<void*>(REBOOTEX_PARAM_OFFSET), 0, 0x100);

	MsLfatFuncs funcs = {
		.msMount = reinterpret_cast<void *>(sceBootLfatMountPatched),
		.msOpen = reinterpret_cast<void *>(sceBootLfatOpenPatched),
		.msRead = reinterpret_cast<void *>(sceBootLfatReadPatched),
		.msClose = reinterpret_cast<void *>(sceBootLfatClosePatched),
	};

	patchIplPayload(&funcs);

	payloadEntry(a0, a1, a2, a3, t0, t1, t2);
}