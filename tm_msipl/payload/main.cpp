#include <cache.h>
#include <ctrl.h>
#include <ff.h>
#include <lowio.h>
#include <ms.h>
#include <tm_common.h>
#include <syscon.h>
#include <serial.h>

#include <type_traits>
#include <cstring>

#include "config.h"
#include "performBoot.h"

constexpr inline u32 FILE_BUFFER_SIZE = 1024;
constexpr inline u32 SECTOR_BUFFER_SIZE = 512;

constexpr inline u32 BUFFER_START_ADDR = 0x40E0000;

auto const g_FileBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR);
auto const g_ConfigBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR+FILE_BUFFER_SIZE);

FATFS fs;
FIL fp;

[[noreturn]] inline void bootFromNand(void *config_buffer = nullptr) {
	if (config_buffer != nullptr) {
		strcpy(reinterpret_cast<char*>(config_buffer), "NAND");
		sdkMsWriteSector(TM_CONFIG_SECTOR, config_buffer);
	}

	performNandBoot();
}

int main() {
	iplSysconInit();

	iplSysconCtrlMsPower(true);

	if constexpr (isDebug) {
		iplSysconCtrlHRPower(true);
		sdkUartHpRemoteInit();
	}

	memset(g_FileBuffer, 0, FILE_BUFFER_SIZE);

	tmTempSetModel(sdkKernelGetModel(iplSysregGetTachyonVersion(), iplSysconGetBaryonVersion()));

	auto const resetPtr = reinterpret_cast<u32*>(0xBC10004C);

	if constexpr (isDebug) {
		printf("START\n");
	}

	if (f_mount(&fs, "ms0:", 1) != FR_OK) { //If a FAT partition can't be found, try to boot from NAND
		if constexpr (isDebug) {
			printf("Mount failed!\n");
		}

		bootFromNand(g_ConfigBuffer);
	}

	if constexpr (isDebug) {
		printf("g_FileBuffer %x\n", g_FileBuffer);
		printf("g_ConfigBuffer %x\n", g_ConfigBuffer);
		printf("resetValue %x\n", *resetPtr);
		// printf("g_PayloadLocation %x\n", g_PayloadLocation);
		// printf("g_PayloadDataLocation %x\n", g_PayloadDataLocation);
		// printf("g_IplLocation %x\n", g_IplLocation);
		// printf("jumpToIpl %x\n", jumpToIpl);
	}

	u32 wakeUpFactor = 0;

	iplSysconGetWakeUpFactor(&wakeUpFactor);

	if constexpr (isDebug) {
		printf("Factor %x\n", wakeUpFactor);
	}

	//PSP is waking up from sleep
	if (wakeUpFactor & 0x80) {
		sdkMsReadSector(TM_CONFIG_SECTOR, g_ConfigBuffer);

		if (strcasecmp(g_ConfigBuffer, "NAND") == 0) { //Last IPL was NAND, boot from it and skip updating config
			if constexpr (isDebug) {
				printf("Wakeup booting NAND\n");
			}
			
			bootFromNand();
		} else if (f_open(&fp, g_ConfigBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK) { //Last IPL was from config, try to load it again
			if constexpr (isDebug) {
				printf("MS booting %s\n", g_ConfigBuffer);
			}

			performMsBoot(fp);
		}

		if constexpr (isDebug) {
			printf("Can't load last file, booting NAND\n");
		}

		//Last IPL from config can't get loaded, try to boot from NAND instead
		bootFromNand(g_ConfigBuffer);
	}

	//Otherwise this is a fresh boot, check pressed buttons
	SceCtrlData pad;

	iplReadBufferPositive(&pad);

	if constexpr (isDebug) {
		printf("Pad buttons %x\n", pad.Buttons);
	}

	//Open config file
	if (f_open(&fp, "ms0:/TM/config.txt", FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		if constexpr (isDebug) {
			printf("Config open failed, boot NAND\n");
		}
		
		bootFromNand(g_ConfigBuffer);
	}

	u32 bytes_read;

	//Read as much of the file as the buffer allows
	if (f_read(&fp, g_FileBuffer, FILE_BUFFER_SIZE, &bytes_read) != FR_OK) {
		if constexpr (isDebug) {
			printf("Config read failed, boot NAND\n");
		}
		
		bootFromNand(g_ConfigBuffer);
	}

	if constexpr (isDebug) {
		printf("Config opened and read\n");
	}

	f_close(&fp);

	u32 pos = 0;

	if (0 == getMatchingConfigEntry(pad.Buttons, g_FileBuffer, bytes_read, g_ConfigBuffer)) {
/*
		if constexpr (isDebug) {
			for(u32 i = 0; i < line_length; i++) {
				printf("%c", g_LineBuffer[i]);
			}
			printf("\n");
			printf("Buttons %x selection %x\n", pad.Buttons, selection);
			printf("Buttons & selection %x\n", pad.Buttons & selection);
		}
*/
		if constexpr (isDebug) {
			printf("Buttons valid for: %s\n", g_ConfigBuffer);
		}

		if (strcasecmp(g_ConfigBuffer, "NAND") == 0) { //Chosen IPL is NAND
			if constexpr (isDebug) {
				printf("Cold booting NAND\n");
			}

			bootFromNand(g_ConfigBuffer);
		} else if (f_open(&fp, g_ConfigBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK) { // Chosen IPL is from config, try to load it
			sdkMsWriteSector(TM_CONFIG_SECTOR, g_ConfigBuffer); // Save current IPL path to config

			if constexpr (isDebug) {
				printf("MS booting %s\n", g_ConfigBuffer);
			}

			performMsBoot(fp);
		}

		if constexpr (isDebug) {
			printf("Failed to load IPL from config\n");
		}

	}

	if constexpr (isDebug) {
		printf("Config ended, defaulting to NAND\n");
	}

	// If the whole config was parsed and nothing was loaded, try to boot from NAND
	bootFromNand(g_ConfigBuffer);
}
