/* 
 * Based on rebootex_config.h from minimum edition - https://github.com/PSP-Archive/minimum_edition
*/

#ifndef __REBOOTEX_CONFIG_H__
#define __REBOOTEX_CONFIG_H__

#include <psptypes.h>

#define REBOOTEX_FILELEN_MAX 0x50
#define REBOOTEX_PARAM_OFFSET	0x88FB0000

typedef struct RebootexParam {
    char	file[REBOOTEX_FILELEN_MAX];//0
	u32		config[0x70/4];//0x50
	int		reboot_index;//0xc0
	int		mem2;
	int		mem8;
	int		k150_flag;
	void*	on_reboot_after;
	void*	on_reboot_buf;
	int		on_reboot_size;
	int		on_reboot_flag;
} RebootexParam;

//	*(u32 *)0x88FB00F0 = size_systemctrl;//

#endif

