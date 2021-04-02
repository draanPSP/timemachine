#include <cache.h>
#include <lowio.h>
#include <ff.h>
#include <ms.h>
#include <serial.h>
#include <tm_common.h>
#include <noBootromPatch.h>

#include <cstring>
#include <type_traits>

constexpr inline u32 IPL_MAX_SIZE = 0x1000000;

constexpr inline u32 SECTOR_SIZE = 512;

char g_ConfigBuffer[SECTOR_SIZE];

FATFS fs;
FIL fp;

//A slightly bigger buffers to fit the additional characters
char iplPath[2*TM_MAX_PATH_LENGTH];
char payloadPath[2*TM_MAX_PATH_LENGTH];

namespace {
	struct PayloadHeader {
		u32 payload_addr; //where to put the payload?
		u32 ipl_addr;     //where to put the Sony IPL?
	};

	inline u32 _makeJ(void const *addr) {
		return 0x08000000 | ((reinterpret_cast<u32>(addr) & 0x0ffffffc) >> 2);
	}

	inline u32 const *_findJrT9Forwards(u32 const *code, u32 const maxLength) {
		constexpr u32 jra_ra_instr = 0x03200008;

		for(u32 i = 0; i < maxLength; ++i) {
			if (code[i] == jra_ra_instr) {
				return &code[i];
			}
		}

		return nullptr;
	}

	[[noreturn]] inline void _jumpTo(u32 const entryPoint) {
		using v_v_function_t = std::add_pointer_t<void()>;
		auto const jump = reinterpret_cast<v_v_function_t const>(entryPoint);

		jump();

		__builtin_unreachable();
	}
}

int main() {
	if constexpr (isDebug) {
		sdkUartHpRemoteInit();
	}

	if constexpr (isDebug) {
		printf("LOADER START\n");
	}

	sdkMsReadSector(TM_CONFIG_SECTOR, g_ConfigBuffer);

	if (f_mount(&fs, "ms0:", 1) != FR_OK) {
		if constexpr (isDebug) {
			printf("Mount failed!\n");
		}
		return -1;
	}

	if constexpr (isDebug) {
		printf("IPL Path: %s\n", g_ConfigBuffer);
	}

	auto const lastSlashPtr = strrchr(g_ConfigBuffer, '/');

	if (!lastSlashPtr) {
		if constexpr (isDebug) {
			printf("Slash not found\n");
		}
		return -1;
	}

	auto const lastSlashPos = static_cast<u32>(lastSlashPtr - g_ConfigBuffer);

	strncpy(iplPath, g_ConfigBuffer, lastSlashPos+1);
	strncpy(payloadPath, g_ConfigBuffer, lastSlashPos+1);

	auto currentIplPathPos = lastSlashPos+1;
	auto currentPayloadPathPos = lastSlashPos+1;

	strncpy(&iplPath[currentIplPathPos], "nandipl", 7);
	currentIplPathPos += 7;

	strncpy(&payloadPath[currentPayloadPathPos], "payload", 7);
	currentPayloadPathPos += 7;

	if constexpr (isDebug) {
		printf("PSP Model: %d\n", tmTempGetModel());
	}

#ifdef MLOADER

	strncpy(&iplPath[currentIplPathPos], "_", 1);
	currentIplPathPos += 1;

	strncpy(&payloadPath[currentPayloadPathPos], "_", 1);
	currentPayloadPathPos += 1;

	switch (tmTempGetModel()) {
		case 0:
			strncpy(&iplPath[currentIplPathPos], "01", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "01", 2);
			break;
		case 1:
			strncpy(&iplPath[currentIplPathPos], "02", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "02", 2);
			break;
		case 2:
			strncpy(&iplPath[currentIplPathPos], "03", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "03", 2);
			break;
		case 3:
			strncpy(&iplPath[currentIplPathPos], "04", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "04", 2);
			break;
		case 4:
			strncpy(&iplPath[currentIplPathPos], "05", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "05", 2);
			break;
		case 6:
			strncpy(&iplPath[currentIplPathPos], "07", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "07", 2);
			break;
		case 8:
			strncpy(&iplPath[currentIplPathPos], "09", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "09", 2);
			break;
		default:
			strncpy(&iplPath[currentIplPathPos], "11", 2);
			strncpy(&payloadPath[currentPayloadPathPos], "11", 2);
			break;
	}

	currentIplPathPos += 2;
	currentPayloadPathPos += 2;

	strncpy(&iplPath[currentIplPathPos], "g", 1);
	currentIplPathPos += 1;

	strncpy(&payloadPath[currentPayloadPathPos], "g", 1);
	currentPayloadPathPos += 1;

#endif

	//the string + null character
	strncpy(&iplPath[currentIplPathPos], ".bin", 5);
	strncpy(&payloadPath[currentPayloadPathPos], ".bin", 5);

	if constexpr (isDebug) {
		printf("Payload Path: %s\n", payloadPath);
		printf("IPL Path: %s\n", iplPath);
	}

	if (f_open(&fp, payloadPath, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		if constexpr (isDebug) {
			printf("Can't load payload\n");
		}
		return -1;
	}

	u32 bytesRead;

	PayloadHeader header;

	f_read(&fp, &header, sizeof(PayloadHeader), &bytesRead);

	auto const payloadPtr = reinterpret_cast<void*>(header.payload_addr);

	f_read(&fp, payloadPtr, IPL_MAX_SIZE, &bytesRead);

	f_close(&fp);

	if (header.ipl_addr == 0) {
		if constexpr (isDebug) {
			printf("IPL not specified, booting payload %x\n", header.payload_addr);
		}

		iplKernelDcacheWritebackInvalidateAll();
		iplKernelIcacheInvalidateAll();

		sdkSync();
		
		_jumpTo(header.payload_addr);
	}

	if (f_open(&fp, iplPath, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		if constexpr (isDebug) {
			printf("Can't load IPL\n");
		}
		return -1;
	}

	auto const iplPtr = reinterpret_cast<void*>(header.ipl_addr);

	f_read(&fp, iplPtr, IPL_MAX_SIZE, &bytesRead);

	f_close(&fp);

	if (tmTempIsNoBootrom()) {
		noBootromPatch(iplPtr);
	}

	//Replace jump to Sony stage2 with jump to our payload
	auto const memory = reinterpret_cast<u32*>(header.ipl_addr);
	auto const jrt9_addr = _findJrT9Forwards(memory, 0x1000);

	if constexpr (isDebug) {
		printf("jr t9 %x\n", jrt9_addr);
	}

	auto const entryPtr = reinterpret_cast<void const*>(header.payload_addr);
	auto const patchPoint0 = const_cast<u32*>(jrt9_addr);

	patchPoint0[0] = _makeJ(entryPtr);
	patchPoint0[1] = 0;

	if constexpr (isDebug) {
		printf("Booting IPL %x\n", header.ipl_addr);
	}

	iplKernelDcacheWritebackInvalidateAll();
	iplKernelIcacheInvalidateAll();

	sdkSync();

	_jumpTo(header.ipl_addr);
}
