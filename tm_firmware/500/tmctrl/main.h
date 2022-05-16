#ifndef __MAIN_H__
#define __MAIN_H__

#include <pspkernel.h>

int SysEventHandler(int ev_id, char *ev_name, void *param, int *result);
int dummy_ok();
int dummy_error();
int Common_IoInit(PspIoDrvArg *arg);
int Common_IoExit(PspIoDrvArg *arg);
int Common_IoOpen(PspIoDrvFileArg *arg, char *filename, int flags, SceMode mode);
int Common_IoClose(PspIoDrvFileArg *arg);
int Common_IoRead(PspIoDrvFileArg *arg, char *data, int len);
int Common_IoWrite(PspIoDrvFileArg *arg, const char *data, int len);
SceOff Common_IoLseek(PspIoDrvFileArg *arg, SceOff ofs, int whence);
int Common_IoIoctl(PspIoDrvFileArg *arg, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
int Common_IoRemove(PspIoDrvFileArg *arg, const char *name);
int Common_IoMkdir(PspIoDrvFileArg *arg, const char *name, SceMode mode);
int Common_IoRmdir(PspIoDrvFileArg *arg, const char *name);
int Common_IoDopen(PspIoDrvFileArg *arg, const char *dirname);
int Common_IoDclose(PspIoDrvFileArg *arg);
int Common_IoDread(PspIoDrvFileArg *arg, SceIoDirent *dir);
int Common_IoGetstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat);
int Common_IoChstat(PspIoDrvFileArg *arg, const char *file, SceIoStat *stat, int bits);
int Common_IoRename(PspIoDrvFileArg *arg, const char *oldname, const char *newname);
int Common_IoChdir(PspIoDrvFileArg *arg, const char *dir);
int Common_IoMount(PspIoDrvFileArg *arg);
int Common_IoUmount(PspIoDrvFileArg *arg);
int Common_IoDevctl(PspIoDrvFileArg *arg, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
int Common_IoUnk21(PspIoDrvFileArg *arg);

#endif