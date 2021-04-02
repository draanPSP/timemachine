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

#include "performBoot.h"

constexpr inline u32 FILE_BUFFER_SIZE = 1024;
constexpr inline u32 LINE_BUFFER_SIZE = 256;
constexpr inline u32 BUTTON_BUFFER_SIZE = 128;
constexpr inline u32 SECTOR_BUFFER_SIZE = 512;

constexpr inline u32 BUFFER_START_ADDR = 0x40E0000;

auto const g_FileBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR);
auto const g_LineBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR+FILE_BUFFER_SIZE);
auto const g_ButtonBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR+FILE_BUFFER_SIZE+LINE_BUFFER_SIZE);
auto const g_ConfigBuffer = reinterpret_cast<char*>(BUFFER_START_ADDR+FILE_BUFFER_SIZE+LINE_BUFFER_SIZE+BUTTON_BUFFER_SIZE);

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
		printf("g_LineBuffer %x\n", g_LineBuffer);
		printf("g_ButtonBuffer %x\n", g_ButtonBuffer);
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

	while (pos < FILE_BUFFER_SIZE) {
		//We have reached the end of the file, end the processing
		if (g_FileBuffer[pos] == 0) {
			break;
		}

		//Read current line
		u32 line_length = 0;
		s32 comment_at = -1;
		while (pos+line_length < FILE_BUFFER_SIZE) {
			// printf("%c\n", g_FileBuffer[pos+line_length]);
			//UNIX newline character combo
			if (g_FileBuffer[pos+line_length] == 0xA) {
				pos += 1;
				break;
			}
			//Windows newline character combo
			if (g_FileBuffer[pos+line_length] == 0xD && g_FileBuffer[pos+line_length+1] == 0xA) {
				pos += 2;
				break;
			}
			//Save the position of first encountered comment character '#', continue consuming characters
			//(but the parser will ignore everyting after)
			if (g_FileBuffer[pos+line_length] == '#' && comment_at < 0) {
				comment_at = line_length;
			}

			g_LineBuffer[line_length] = g_FileBuffer[pos+line_length];
			line_length++;
		}
		//Advance the file pointer, whole line was read
		pos += line_length;

		//Comment was placed at the current line, trim the parsed line length to '#' character position
		if (comment_at >= 0) {
			line_length = comment_at;
		}

		//If the line is empty, try the next line
		if (line_length == 0) {
			continue;
		}

		//Proper line should end with ';', otherwise try the next line
		if (g_LineBuffer[line_length-1] != ';') {
			continue;
		}

		//Find '=' token to later parse IPL path regardless if button parsing is interrupted by an error or not
		u32 equals_pos;
		for (equals_pos = 0; equals_pos < line_length; equals_pos++) {
			if (g_LineBuffer[equals_pos] == '=') {
				break;
			}
		}

		//No '=' found, try the next line
		if (equals_pos == line_length) {
			continue;
		}

		g_LineBuffer[line_length] = '\0';

		//Parse buttons
		u32 selection = 0;
		u32 button_length = 0, line_pos = 0;
		while (line_pos+button_length < line_length) {
			if (g_LineBuffer[line_pos+button_length] == ' ' || g_LineBuffer[line_pos+button_length] == '+') {
				//Whole button name was read. NULL-terminate it and check if it matches any of the valid buttons
				g_ButtonBuffer[button_length] = '\0';

				if(strcasecmp(g_ButtonBuffer, "UP") == 0) {
					selection |= PSP_CTRL_UP;
				} else if(strcasecmp(g_ButtonBuffer, "RIGHT") == 0) {
					selection |= PSP_CTRL_RIGHT;
				} else if(strcasecmp(g_ButtonBuffer, "DOWN") == 0) {
					selection |= PSP_CTRL_DOWN;
				} else if(strcasecmp(g_ButtonBuffer, "LEFT") == 0) {
					selection |= PSP_CTRL_LEFT;
				} else if(strcasecmp(g_ButtonBuffer, "TRIANGLE") == 0) {
					selection |= PSP_CTRL_TRIANGLE;
				} else if(strcasecmp(g_ButtonBuffer, "CIRCLE") == 0) {
					selection |= PSP_CTRL_CIRCLE;
				} else if(strcasecmp(g_ButtonBuffer, "CROSS") == 0) {
					selection |= PSP_CTRL_CROSS;
				} else if(strcasecmp(g_ButtonBuffer, "SQUARE") == 0) {
					selection |= PSP_CTRL_SQUARE;
				} else if(strcasecmp(g_ButtonBuffer, "SELECT") == 0) {
					selection |= PSP_CTRL_SELECT;
				} else if((strcasecmp(g_ButtonBuffer, "LTRIGGER")) == 0 || (strcasecmp(g_ButtonBuffer, "L") == 0)) {
					selection |= PSP_CTRL_LTRIGGER;
				} else if((strcasecmp(g_ButtonBuffer, "RTRIGGER")) == 0 || (strcasecmp(g_ButtonBuffer, "R") == 0)) {
					selection |= PSP_CTRL_RTRIGGER;
				} else if(strcasecmp(g_ButtonBuffer, "START") == 0) {
					selection |= PSP_CTRL_START;
				} else if(strcasecmp(g_ButtonBuffer, "HOME") == 0) {
					selection |= PSP_CTRL_HOME;
				} else if(strcasecmp(g_ButtonBuffer, "WLAN") == 0) {
					selection |= PSP_CTRL_WLAN_UP;
				} else if(strcasecmp(g_ButtonBuffer, "VOLDOWN") == 0) {
					selection |= PSP_CTRL_VOLDOWN;
				} else if(strcasecmp(g_ButtonBuffer, "VOLUP") == 0) {
					selection |= PSP_CTRL_VOLUP;
				} else if(strcasecmp(g_ButtonBuffer, "HPREMOTE") == 0) {
					selection |= PSP_CTRL_REMOTE;
				} else if(strcasecmp(g_ButtonBuffer, "NOTE") == 0) {
					selection |= PSP_CTRL_NOTE;
				} else if(strcasecmp(g_ButtonBuffer, "LCD") == 0) {
					selection |= PSP_CTRL_SCREEN;
				} else if(strcasecmp(g_ButtonBuffer, "NOTHING") == 0) {
					selection = 0;
				} else {
					//Invalid button name. Stop parsing buttons and continue to parse IPL path
					break;
				}

				//If the next character is '+', skip it and continue the button combo
				if(g_LineBuffer[line_pos+button_length] == '+') {
					line_pos += button_length+1;
					button_length = 0;
				} else {
					//Otherwise we are finished, continue to parse IPL path
					break;
				}
			} else {
				g_ButtonBuffer[button_length] = g_LineBuffer[line_pos+button_length];
				button_length++;
			}
		}

		//Start after the '=' character and look for '"'
		for (line_pos = equals_pos+1; line_pos < line_length; ++line_pos) {
			if (g_LineBuffer[line_pos] == '"') {
				break;
			}
		}
		//No '"' found, try the next line
		if (line_pos == line_length) {
			continue;
		}

		//Look for the other '"'
		u32 endpos;
		for (endpos = line_pos+1; endpos < line_length; ++endpos) {
			if (g_LineBuffer[endpos] == '"') {
				break;
			}
		}
		//No second '"' found, try the next line
		if (endpos == line_length) {
			continue;
		}

		if constexpr (isDebug) {
			for(u32 i = 0; i < line_length; i++) {
				printf("%c", g_LineBuffer[i]);
			}
			printf("\n");
			printf("Buttons %x selection %x\n", pad.Buttons, selection);
			printf("Buttons & selection %x\n", pad.Buttons & selection);
		}

		//Do we have a match between the config line and pressed buttons?
		if ((pad.Buttons & selection) == selection) {
			//Copy the IPL as (potential) boot target
			u32 const size = endpos - (line_pos+1);
			strncpy(g_ConfigBuffer, &g_LineBuffer[line_pos+1], size > TM_MAX_PATH_LENGTH ? TM_MAX_PATH_LENGTH : size);
			g_ConfigBuffer[size] = '\0';

			if constexpr (isDebug) {
				printf("Buttons valid for: %s\n", g_ConfigBuffer);
			}

			if (strcasecmp(g_ConfigBuffer, "NAND") == 0) { //Chosen IPL is NAND
				if constexpr (isDebug) {
					printf("Cold booting NAND\n");
				}

				bootFromNand(g_ConfigBuffer);
			} else if (f_open(&fp, g_ConfigBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK) { //Chosen IPL is from config, try to load it
				sdkMsWriteSector(TM_CONFIG_SECTOR, g_ConfigBuffer); //Save current IPL path to config

				if constexpr (isDebug) {
					printf("MS booting %s\n", g_ConfigBuffer);
				}
				
				performMsBoot(fp);
			}

			if constexpr (isDebug) {
				printf("Failed to load IPL from config\n");
			}

			//Failed to load the specified IPL. Continue to the next line
		}
	}

	if constexpr (isDebug) {
		printf("Config ended, defaulting to NAND\n");
	}

	//If the whole config was parsed and nothing was loaded, try to boot from NAND
	bootFromNand(g_ConfigBuffer);
}
