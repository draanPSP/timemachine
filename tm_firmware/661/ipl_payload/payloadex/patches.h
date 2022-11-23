#pragma once

#include <psptypes.h>

struct PayloadPatches {
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
	struct PayloadPatches payloadPatches;
	struct LoadCorePatches loadCorePatches;
};

static const struct Patches patches = {
#if PSP_MODEL == 0
	.payloadPatches = {
		.BootLfatMountPatch	= 0x88603fc0,
        .BootLfatOpenPatch	= 0x88603fd0,
		.BootLfatReadPatch	= 0x88604038,
		.BootLfatClosePatch	= 0x88604058,
		.CheckPspConfigPatch= 0x88609ef4,
		.KdebugPatchAddr	= 0x8860bd70,
		.BtHeaderPatchAddr	= 0x8860a9f4,
		.LfatMountPatchAddr	= 0x88603fc8,
		.LfatSeekPatchAddr1	= 0x88604018,
		.LfatSeekPatchAddr2	= 0x88604028,
		.LoadCorePatchAddr	= 0x886034ac,
		.HashCheckPatchAddr	= 0x88603a6c,
		.SigcheckPatchAddr  = 0x88600b48,
	},

#elif (PSP_MODEL == 1)
	.payloadPatches = {
        .BootLfatMountPatch	= 0x88604088,
		.BootLfatOpenPatch	= 0x88604098,
		.BootLfatReadPatch	= 0x88604100,
		.BootLfatClosePatch	= 0x88604120,
		.CheckPspConfigPatch= 0x88609fb4,
		.KdebugPatchAddr	= 0x8860be30,
		.BtHeaderPatchAddr	= 0x8860aab4,
		.LfatMountPatchAddr	= 0x88604090,
		.LfatSeekPatchAddr1	= 0x886040e0,
		.LfatSeekPatchAddr2	= 0x886040f0,
		.LoadCorePatchAddr	= 0x88603574,
		.HashCheckPatchAddr	= 0x88603b34,
		.SigcheckPatchAddr  = 0x88600bd8,
	},
#endif
	.loadCorePatches = {
		.ModuleOffsetAddr	= 0x00000af8,
		.SigcheckPatchAddr	= 0x00005994,
		.SigcheckFuncAddr	= 0x00007824,
		.DecryptPatchAddr	= 0x00005970,
		.DecryptFuncAddr	= 0x0000783c,
	},
};