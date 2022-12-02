/* 
 * Based on payloadex_patch_addr.h/rebootex_patch_addr.h from minimum edition - https://github.com/PSP-Archive/minimum_edition
*/

#pragma once

#include <psptypes.h>

struct RebootPatches {
	u32 BootLfatMountPatch;
	u32 BootLfatOpenPatch;
	u32 BootLfatReadPatch;
	u32 BootLfatClosePatch;
	u32 CheckPspConfigPatch;
	u32 KdebugPatchAddr;
	u32 BtHeaderPatchAddr;
	u32 LfatMountPatchAddr;
	u32 LfatSeekPatchAddr1;
	u32 LfatSeekPatchAddr2;
	u32 LoadCorePatchAddr;
	u32 HashCheckPatchAddr;
	u32 SigcheckPatchAddr;
};

struct LoadCorePatches {
	u32 ModuleOffsetAddr;
	u32 SigcheckPatchAddr;
	u32 SigcheckFuncAddr;
	u32 DecryptPatchAddr;
	u32 DecryptFuncAddr;
};

struct Patches {
	struct RebootPatches rebootPatches;
	struct LoadCorePatches loadCorePatches;
};

struct MsLfatFuncs {
    void *msMount;
    void *msOpen;
    void *msRead;
    void *msClose;
};

static const struct Patches patches = {
#if PSP_MODEL == 0
#if defined PAYLOADEX
	.rebootPatches = {
		.BootLfatMountPatch	= 0x88603fc0,
        .BootLfatOpenPatch	= 0x88603fd0,
		.BootLfatReadPatch	= 0x88604038,
		.BootLfatClosePatch	= 0x88604058,
		.CheckPspConfigPatch= 0x88609ef4,
		.KdebugPatchAddr	= 0x8860bd70,
		.LfatMountPatchAddr	= 0x88603fc8,
		.LfatSeekPatchAddr1	= 0x88604018,
		.LfatSeekPatchAddr2	= 0x88604028,
		.LoadCorePatchAddr	= 0x886034ac,
		.HashCheckPatchAddr	= 0x88603a6c,
		.SigcheckPatchAddr  = 0x88600b48,
	},
#elif defined REBOOTEX
	.rebootPatches = {
		.BootLfatMountPatch	= 0x886027b0,
        .BootLfatOpenPatch	= 0x886027c4,
		.BootLfatReadPatch	= 0x88602834,
		.BootLfatClosePatch	= 0x88602860,
		.CheckPspConfigPatch= 0x88605780,
		.KdebugPatchAddr	= 0x88603880,
		.LfatMountPatchAddr	= 0x886027bc,
		.LfatSeekPatchAddr1	= 0x88602810,
		.LfatSeekPatchAddr2	= 0x88602828,
		.LoadCorePatchAddr	= 0x88605638,
		.HashCheckPatchAddr	= 0x88607390,
		.SigcheckPatchAddr  = 0x88601620,
	},
#endif

#elif (PSP_MODEL == 1)
#if defined PAYLOADEX
	.rebootPatches = {
        .BootLfatMountPatch	= 0x88604088,
		.BootLfatOpenPatch	= 0x88604098,
		.BootLfatReadPatch	= 0x88604100,
		.BootLfatClosePatch	= 0x88604120,
		.CheckPspConfigPatch= 0x88609fb4,
		.KdebugPatchAddr	= 0x8860be30,
		.LfatMountPatchAddr	= 0x88604090,
		.LfatSeekPatchAddr1	= 0x886040e0,
		.LfatSeekPatchAddr2	= 0x886040f0,
		.LoadCorePatchAddr	= 0x88603574,
		.HashCheckPatchAddr	= 0x88603b34,
		.SigcheckPatchAddr  = 0x88600bd8,
	},
#elif defined REBOOTEX
	.rebootPatches = {
        .BootLfatMountPatch	= 0x88602878,
		.BootLfatOpenPatch	= 0x8860288c,
		.BootLfatReadPatch	= 0x886028fc,
		.BootLfatClosePatch	= 0x88602928,
		.CheckPspConfigPatch= 0x88605840,
		.KdebugPatchAddr	= 0x88603948,
		.LfatMountPatchAddr	= 0x88602884,
		.LfatSeekPatchAddr1	= 0x886028d8,
		.LfatSeekPatchAddr2	= 0x886028f0,
		.LoadCorePatchAddr	= 0x886056f8,
		.HashCheckPatchAddr	= 0x88607450,
		.SigcheckPatchAddr  = 0x886016b0,
	},
#endif
#endif
	.loadCorePatches = {
		.ModuleOffsetAddr	= 0x00000af8,
		.SigcheckPatchAddr	= 0x00005994,
		.SigcheckFuncAddr	= 0x00007824,
		.DecryptPatchAddr	= 0x00005970,
		.DecryptFuncAddr	= 0x0000783c,
	},
};

#ifdef PAYLOADEX
void patchIplPayload(MsLfatFuncs *funcs);
#elif defined REBOOTEX
void patchRebootBin(MsLfatFuncs *funcs);
#endif
