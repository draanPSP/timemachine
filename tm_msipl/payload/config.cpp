#include <cstring>
#include <ctrl.h>
#include <serial.h>
#include <tm_common.h>
#include <type_traits>

static const u32 INVALID_BUTTON = 0xFFFFFFFF;

static const u32 usefulButtonsMask = ~(PSP_CTRL_HOLD | PSP_CTRL_WLAN_UP | PSP_CTRL_REMOTE | PSP_CTRL_DISC | PSP_CTRL_MS);

static bool validateConfigEntry(char *line, u32 line_len, char **combo, char **config) {
	// format of valid config line is:
	//     BTN+BTN = "filename";
	int num_quotes = 0;
	*combo = *config = NULL;

	for (u32 i = 0; i < line_len; ++i) {
		// stop processing at a comment marker unless it's a part of the value
		if (line[i] == '#' && num_quotes != 1)
			break;
		// = marks the border between key and value
		if (line[i] == '=') {
			*combo = line;
			line[i] = '\0';
			// eat trailing spaces
			for (s32 j = i-1; j > 0 && line[j] == ' '; --j)
				line[j] = '\0';
			// eat leading spaces
			while (**combo == ' ') {
				*combo++;
			}
		// quotes surround our value
		} else if (line[i] == '"') {
			if (!*combo) // can only get a quote after the combo
				break;
			if (++num_quotes == 1) {
				*config = line + i + 1;
			} else {
				// if we have more than one quote then we have our value
				// and are just waiting on a semicolon for success

				// eat the quote
				line[i] = '\0';
			}
		// line must terminate with a semi-colon
		} else if (line[i] == ';') {
			// if we have our key, and a quote wrapped value then GO GO GO
			if (*combo && *config && num_quotes > 1)
				return true;
		}
	}

	return false;
}

static u32 maskForButton(char *button) {
	if(strcasecmp(button, "UP") == 0) {
		return PSP_CTRL_UP;
	} else if(strcasecmp(button, "RIGHT") == 0) {
		return PSP_CTRL_RIGHT;
	} else if(strcasecmp(button, "DOWN") == 0) {
		return PSP_CTRL_DOWN;
	} else if(strcasecmp(button, "LEFT") == 0) {
		return PSP_CTRL_LEFT;
	} else if(strcasecmp(button, "TRIANGLE") == 0) {
		return  PSP_CTRL_TRIANGLE;
	} else if(strcasecmp(button, "CIRCLE") == 0) {
		return PSP_CTRL_CIRCLE;
	} else if(strcasecmp(button, "CROSS") == 0) {
		return PSP_CTRL_CROSS;
	} else if(strcasecmp(button, "SQUARE") == 0) {
		return PSP_CTRL_SQUARE;
	} else if(strcasecmp(button, "SELECT") == 0) {
		return PSP_CTRL_SELECT;
	} else if((strcasecmp(button, "LTRIGGER")) == 0 || (strcasecmp(button, "L") == 0)) {
		return PSP_CTRL_LTRIGGER;
	} else if((strcasecmp(button, "RTRIGGER")) == 0 || (strcasecmp(button, "R") == 0)) {
		return PSP_CTRL_RTRIGGER;
	} else if(strcasecmp(button, "START") == 0) {
		return PSP_CTRL_START;
	} else if(strcasecmp(button, "HOME") == 0) {
		return PSP_CTRL_HOME;
	} else if(strcasecmp(button, "WLAN") == 0) {
		return PSP_CTRL_WLAN_UP;
	} else if(strcasecmp(button, "VOLDOWN") == 0) {
		return PSP_CTRL_VOLDOWN;
	} else if(strcasecmp(button, "VOLUP") == 0) {
		return PSP_CTRL_VOLUP;
	} else if(strcasecmp(button, "HPREMOTE") == 0) {
		return PSP_CTRL_REMOTE;
	} else if(strcasecmp(button, "NOTE") == 0) {
		return PSP_CTRL_NOTE;
	} else if(strcasecmp(button, "LCD") == 0) {
		return PSP_CTRL_SCREEN;
	} else if(strcasecmp(button, "NOTHING") == 0) {
		return 0;
	}

	return INVALID_BUTTON;
}

static u32 maskForButtonCombo(char *combo) {
	char *ele_start = combo;
	char *ele_next = ele_start;
	u32 result = 0;
	
	if (!combo || strlen(combo) == 0)
		return INVALID_BUTTON;

	while (*ele_next != '\0') {
		if (*ele_next == '+') {
			*ele_next = '\0';

			// build up the mask for this button
			result |= maskForButton(ele_start);
			if (result == INVALID_BUTTON)
				return result;

			// restore '+' so we could print the line later
			*ele_next = '+';

			// setup the next sub-string
			ele_start = ++ele_next;
		} else {
			ele_next++;
		}
	}

	// ensure we've parsed everything
	if (ele_start != ele_next) {
		result |= maskForButton(ele_start);
	}

	return result;
}

static bool isMatchingConfigEntry(char *line, u32 lineLen, u32 buttons, char * const configOutput) {
	char *combo;
	char *config;

	if (validateConfigEntry(line, lineLen, &combo, &config)) {
		u32 mask = maskForButtonCombo(combo);

		// check if the combo mask matches the useful subset of what's currently 'pressed'
		if (mask == (usefulButtonsMask & buttons)) {
			if constexpr (isDebug) {
				printf("Config line: %s\n", line);
				printf("Buttons: 0x%08x Selection: 0x%08x\n", usefulButtonsMask & buttons, mask);
			}
			strncpy(configOutput, config, TM_MAX_PATH_LENGTH);
			return true;
		}
	}

	return false;
}

int getMatchingConfigEntry(u32 buttons, char * const inputBuffer, u32 inputBufferSize, char * const configOutput) {
	char *line_begin = inputBuffer;
	char *line_end = NULL;

	// find a string
	for (u32 i = 0; i < inputBufferSize; ++i) {
		if (inputBuffer[i] == '\r' || inputBuffer[i] == '\n') {
			line_end = inputBuffer + i;
			if (line_end - line_begin > 0) {
				if (isMatchingConfigEntry(line_begin, line_end - line_begin, buttons, configOutput))
					return 0;
			}
			line_begin = inputBuffer + i + 1;
		}
	}

	return 1;
}
