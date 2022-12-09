#ifndef __MAIN_H__
#define __MAIN_H__

typedef struct
{
	int magic; /* 0x47434553 */
	int hidecorrupt;
	int	skiplogo;
	int umdactivatedplaincheck;
	int gamekernel150;
	int executebootbin;
	int startupprog;
	int umdmode;
	int useisofsonumdinserted;
	int	vshcpuspeed; 
	int	vshbusspeed; 
	int	umdisocpuspeed; 
	int	umdisobusspeed; 
	int fakeregion;
	int freeumdregion;
	int	hardresetHB; 
	int usbdevice;
	int novshmenu;
} SEConfig;

typedef int (* APRS_EVENT)(char *modname, u8 *modbuf);

APRS_EVENT sctrlHENSetOnApplyPspRelSectionEvent(APRS_EVENT func);

int sceKernelLzrcDecode(u8 *dest, int destSize, u8 *src, int unk);

#define CONFIG_MAGIC 0x47434553

#endif