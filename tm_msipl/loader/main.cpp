#include <preipl.h>
#include <lowio.h>
#include <cache.h>
#include <type_traits>

extern "C" {
	#include <heatshrink_decoder.h>
}

inline auto const g_SectorBuffer = reinterpret_cast<std::uint8_t*>(0x04000000);
constexpr inline std::uint32_t SECTOR_SIZE = 512;
inline auto const g_CodeDestination = reinterpret_cast<std::uint8_t*>(PAYLOAD_ENTRY_ADDR);

inline auto const payloadEntryPtr = reinterpret_cast<i_v_function_t const>(PAYLOAD_ENTRY_ADDR);

constexpr inline std::uint32_t MS_PAYLOAD_START_SECTOR = 0x18;
constexpr inline std::uint32_t FAT_TABLE_SECTOR = 0x3F;

constexpr inline std::uint32_t TACHYON_0x140000 = 0x140000;
constexpr inline std::uint32_t TACHYON_0x400000 = 0x400000;
constexpr inline std::uint32_t TACHYON_0x600000 = 0x600000;

namespace {
	inline void _setTimestampRegister(std::uint32_t const &tachyonVer) {
		std::uint32_t timestamp;

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

	auto const tachyonVer = iplSysregGetTachyonVersion();

	auto const preIplDcacheWritebackInvalidateAll = tachyonVer >= TACHYON_0x600000 ? newPreIplDcacheWritebackInvalidateAll : oldPreIplDcacheWritebackInvalidateAll;
	auto const preIplIcacheInvalidateAll = tachyonVer >= TACHYON_0x600000 ? newPreIplIcacheInvalidateAll : oldPreIplIcacheInvalidateAll;
	auto const preIplMsReadSector = tachyonVer >= TACHYON_0x600000 ? newPreIplMsReadSector : oldPreIplMsReadSector;

	_setTimestampRegister(tachyonVer);

	std::uint32_t decompressed = 0;

	for(std::uint32_t currentSector = MS_PAYLOAD_START_SECTOR; currentSector < FAT_TABLE_SECTOR; ++currentSector) {
		preIplMsReadSector(currentSector, g_SectorBuffer);

		std::size_t count;

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
