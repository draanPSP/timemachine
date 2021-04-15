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
	using v_iiii_function_t = std::add_pointer_t<void(s32,s32,s32,s32)>;
	using module_start_function_t = std::add_pointer_t<int(SceSize, void*, void*)>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x886007C0);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x8860022C);
	inline auto const rebootEntryPtr = reinterpret_cast<v_iiii_function_t const>(0x88600000);

	[[noreturn]] inline void rebootEntry(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {
		rebootEntryPtr(a0, a1, a2, a3);

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
		//if (f_open(&fp, filename, FA_OPEN_EXISTING | FA_READ) == FR_OK)
		return 0;
	}

	int sceBootLfatReadPatched(void *buffer, int buffer_size) {
		// u32 bytes_read;
		// if (f_read(&fp, buffer, buffer_size, &bytes_read) == FR_OK)
		return 0;
	}

	int sceBootLfatClosePatched() {
		// f_close(&fp);
		return 0;
	}

	int checkPSPHeaderPatched() {
		return 0;
	}

	v_v_function_t memlmdOriginal;

	int memlmdDecrypt() {
		return 0;
	}

	int loadCoreModuleStartPatched(SceSize args, void *argp, void *unk, module_start_function_t module_start) {
		u32 const text_addr = reinterpret_cast<u32>(module_start) - 0xC74;

		MAKE_CALL(text_addr + 0x41D0, memlmdDecrypt);
		MAKE_CALL(text_addr + 0x68F8, memlmdDecrypt);

		memlmdOriginal = reinterpret_cast<v_v_function_t>(text_addr + 0x81D4);

		// disable unsign check
		_sw(0x1021, text_addr + 0x691C); // move $v0, $zero
		_sw(0x1021, text_addr + 0x694C); // move $v0, $zero
		_sw(0x1021, text_addr + 0x69E4); // move $v0, $zero

		clearCaches();

		return module_start(args, argp, unk);
	}

	u32 config1, config2, config3, config4, config5, config6;
}

int main(s32 const a0, s32 const a1, s32 const a2, s32 const a3) {
	MAKE_CALL(0x8860200C, sceBootLfatMountPatched);
	MAKE_CALL(0x88602020, sceBootLfatOpenPatched);
	MAKE_CALL(0x88602090, sceBootLfatReadPatched);
	MAKE_CALL(0x886020BC, sceBootLfatClosePatched);

	// patch ~PSP header check
	_sw(0xAFA50000, 0x88605030); // sw $a1, ($sp)
	_sw(0x20A30000, 0x88605034); // addi $v1, $a1, 0

	MAKE_CALL(0x88606A90, checkPSPHeaderPatched);

	MAKE_FUNCTION_RETURN(0x886030E0, 1);

	_sw(0, 0x88602018);
	_sw(0, 0x8860206C);
	_sw(0, 0x88602084);

	// Patch the call to LoadCore module_start 
	_sw(0x113821, 0x88604EF0); // jr $t7 -> move $a3, $t7 // a3 = LoadCore module_start 
	MAKE_JUMP(0x88604EF4, loadCoreModuleStartPatched); // move $sp, $s4 -> j PatchLoadCore
	_sw(0x2A0E821, 0x88604EF8); //nop -> move $sp, $s4

	_sw(0, 0x88606D38);

	MAKE_FUNCTION_RETURN0(0x886013CC);
	
	//Read systemctrl config ?
	config1 = _lw(0x88FB00D0);
	config2 = _lw(0x88FB00D4);
	config3 = _lw(0x88FB00D8);
	config4 = _lw(0x88FB00DC);
	config5 = 0;
	config6 = 0;

	clearCaches();

	// Hmm?
	iplSysregSpiClkEnable(ClkSpi::SPI1);

	iplSysregGpioIoEnable(GpioPort::SYSCON_REQUEST);
	iplSysregGpioIoEnable(GpioPort::MS_LED);
	iplSysregGpioIoEnable(GpioPort::WLAN_LED);
	sdkSync();

	iplSysconInit();
	iplSysconCtrlMsPower(true);
	
	rebootEntry(a0, a1, a2, a3);
}