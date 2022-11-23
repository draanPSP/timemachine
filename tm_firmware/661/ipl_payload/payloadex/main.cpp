#include <string.h>

#include <cache.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>

#include "patches.h"

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using v_iiiiiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*)>;
	using nand_dec_function_t = std::add_pointer_t<int(u8*, u32)>;
	using decrypt_function_t = std::add_pointer_t<int(u8*, u32, s32*)>;

	inline auto const payloadEntryPtr = reinterpret_cast<v_iiiiiii_function_t const>(0x88600000);
	nand_dec_function_t nandDecryptPtr = NULL;
	decrypt_function_t decryptPtr = NULL;

	u32 loadCoreTextAddr;

	char path[260];

	[[noreturn]] inline void payloadEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {
		payloadEntryPtr(a0, a1, a2, a3, t0, t1, t2);

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

#if PSP_MODEL == 0
		if(strcmp(filename, "/kd/pspbtcnf.bin") == 0) {
			strcpy(filename, "/kd/pspbtjnf.bin");
		}
#elif PSP_MODEL == 1
		if(strcmp(filename, "/kd/pspbtcnf_02g.bin") == 0) {
			strcpy(filename, "/kd/pspbtjnf_02g.bin");
		}
#endif
		else if(strcmp(filename, "/kd/lfatfs.prx") == 0) {
			strcpy(filename, "/tmctrl661.prx");
		}

		strcpy(path, "/TM/661");
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

	int nandDecryptPatched(u8 *buf, u32 size) {

		for (int i = 0; i < 0x30; i++) {
			if (buf[0xd4 + i] != 0)
				return nandDecryptPtr(buf, size);
		}

		return 0;
	}

	void scrambleSimple(u32 *target, u32 *seed, s32 size) {

		u32 *end_buffer = target + size/sizeof(int);

		while( target < end_buffer )
		{
			*target ^= *seed;

			seed++;
			target++;
		}
	}

	void scramble(u8 *target, u32 size, u8 *seed, s32 seed_size) {

		int seed_offset = 0;
		u8 *end_buffer = target + size;

		while( target < end_buffer )
		{
			if( seed_offset >= seed_size )
				seed_offset = 0;

			*target ^= seed[seed_offset];
			seed_offset++;
			target++;	
		}
	}

	int decryptPatched(u8 *buf, u32 size, s32 *s) {

		u32 tag = *reinterpret_cast<u32 *>(buf + 0x130);
		s32 compSize = *reinterpret_cast<s32 *>(buf + 0xb0);
		u32 *seed = reinterpret_cast<u32 *>(buf + 0xc0);
		u8 *seed2 = reinterpret_cast<u8 *>(buf + 0x10c);
		u32 *data = reinterpret_cast<u32 *>(buf + 0x150);

		if (tag == 0xC6BA41D3)
		{
			scrambleSimple(data, seed, 0x10);
			scramble(buf + 0x150, compSize, seed2, 0x20);
			memcpy(buf, buf + 0x150, compSize);
			*s = compSize;

			MAKE_CALL(loadCoreTextAddr + patches.loadCorePatches.DecryptPatchAddr, decryptPtr);

			clearCaches();

			return 0;
		}

		return decryptPtr(buf, size, s);
	}

	int loadCoreModuleStartPatched(SceSize args, void *argp, module_start_function_t module_start) {

		loadCoreTextAddr = reinterpret_cast<u32>(module_start) - patches.loadCorePatches.ModuleOffsetAddr;

		MAKE_CALL(loadCoreTextAddr + patches.loadCorePatches.SigcheckPatchAddr, nandDecryptPatched);
		MAKE_CALL(loadCoreTextAddr + patches.loadCorePatches.DecryptPatchAddr, decryptPatched);

		nandDecryptPtr = reinterpret_cast<nand_dec_function_t>(loadCoreTextAddr + patches.loadCorePatches.SigcheckFuncAddr);
		decryptPtr = reinterpret_cast<decrypt_function_t>(loadCoreTextAddr + patches.loadCorePatches.DecryptFuncAddr);

		clearCaches();

		return module_start(8, argp);
	}
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3, s32 const t0, s32 const t1, s32 const t2) {

	memset(reinterpret_cast<void*>(0x88fb0000), 0, 0x100);

	_sw(0x01e03021, patches.payloadPatches.LoadCorePatchAddr);
	MAKE_JUMP(patches.payloadPatches.LoadCorePatchAddr + 0x8, loadCoreModuleStartPatched);

	_sw(0xafa50000, patches.payloadPatches.CheckPspConfigPatch);
	_sw(0x20a30000, patches.payloadPatches.CheckPspConfigPatch + 0x4);

	MAKE_FUNCTION_RETURN0(patches.payloadPatches.SigcheckPatchAddr);

	_sw(0, patches.payloadPatches.LfatMountPatchAddr);
	_sw(0, patches.payloadPatches.LfatSeekPatchAddr1);
	_sw(0, patches.payloadPatches.LfatSeekPatchAddr2);
	_sw(0, patches.payloadPatches.HashCheckPatchAddr);

	MAKE_FUNCTION_RETURN(patches.payloadPatches.KdebugPatchAddr, 1);

	MAKE_CALL(patches.payloadPatches.BootLfatMountPatch, sceBootLfatMountPatched);
	MAKE_CALL(patches.payloadPatches.BootLfatOpenPatch, sceBootLfatOpenPatched);
	MAKE_CALL(patches.payloadPatches.BootLfatReadPatch, sceBootLfatReadPatched);
	MAKE_CALL(patches.payloadPatches.BootLfatClosePatch, sceBootLfatClosePatched);

	clearCaches();

	payloadEntry(a0, a1, a2, a3, t0, t1, t2);
}