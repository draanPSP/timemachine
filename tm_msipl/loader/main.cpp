#include <preipl.h>
#include <lowio.h>
#include <cache.h>
#include <type_traits>

extern "C" {
	#include <heatshrink_decoder.h>
}

inline auto const g_SectorBuffer = reinterpret_cast<u8*>(0x04000000);
constexpr inline u32 SECTOR_SIZE = 512;
inline auto const g_CodeDestination = reinterpret_cast<u8*>(PAYLOAD_ENTRY_ADDR);

inline auto const payloadEntryPtr = reinterpret_cast<i_v_function_t const>(PAYLOAD_ENTRY_ADDR);

constexpr inline u32 MS_PAYLOAD_START_SECTOR = 0x18;
constexpr inline u32 FAT_TABLE_SECTOR = 0x3F;

constexpr inline u32 TACHYON_0x140000 = 0x140000;
constexpr inline u32 TACHYON_0x400000 = 0x400000;
constexpr inline u32 TACHYON_0x600000 = 0x600000;

namespace {
	inline void _setTimestampRegister(u32 const &tachyonVer) {
		u32 timestamp;

		if (tachyonVer >= TACHYON_0x600000) {
			timestamp = TACHYON_0x600000_BOOTROM_TIMESTAMP;
		} else if (tachyonVer >= TACHYON_0x400000) {
			timestamp = TACHYON_0x400000_BOOTROM_TIMESTAMP;
		} else {
			timestamp = TACHYON_0x140000_BOOTROM_TIMESTAMP;
		}

		asm ("ctc0 %0, $17\n" :: "r" (timestamp));
	}

	[[noreturn]] inline void _payloadEntry() {
		payloadEntryPtr();

		__builtin_unreachable();
	}
}

int main() {
	heatshrink_decoder hsd;
	heatshrink_decoder_reset(&hsd);

	u32 const tachyonVer = iplSysregGetTachyonVersion();

#ifdef RESET_EXPLOIT
	//Due to the reset exploit, we accidentally enabled
	//more devices than should be enabled at this point.
	//bits high (device in reset) are for:
	//USB_HOST, ATA_HDD, MS_1, ATA, USB, AVC, VME, ME
	memoryK1(REG_RESET_ENABLE) |= 0x32F4;
#endif

	auto const preIplDcacheWritebackInvalidateAll = tachyonVer >= TACHYON_0x600000 ? newPreIplDcacheWritebackInvalidateAll : oldPreIplDcacheWritebackInvalidateAll;
	auto const preIplIcacheInvalidateAll = tachyonVer >= TACHYON_0x600000 ? newPreIplIcacheInvalidateAll : oldPreIplIcacheInvalidateAll;
	auto const preIplMsReadSector = tachyonVer >= TACHYON_0x600000 ? newPreIplMsReadSector : oldPreIplMsReadSector;

	_setTimestampRegister(tachyonVer);

	u32 decompressed = 0;

	for(u32 currentSector = MS_PAYLOAD_START_SECTOR; currentSector < FAT_TABLE_SECTOR; ++currentSector) {
		preIplMsReadSector(currentSector, g_SectorBuffer);

		u32 count;

		heatshrink_decoder_sink(&hsd, g_SectorBuffer, SECTOR_SIZE, &count);

		HSD_poll_res pres;

		do {
			pres = heatshrink_decoder_poll(&hsd, &g_CodeDestination[decompressed], SECTOR_SIZE, &count);
			decompressed += count;
		} while(pres == HSDR_POLL_MORE);
	}

	preIplDcacheWritebackInvalidateAll();
	preIplIcacheInvalidateAll();

	sdkSync();

	_payloadEntry();
}