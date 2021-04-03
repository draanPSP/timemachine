#include <noBootromPatch.h>
#include <serial.h>
#include <cstring>
#include <tm_common.h>
#include <tableHash.h>

namespace {
	constexpr inline u32 JAL_TYPE = 0x0C000000;
	constexpr inline u32 LW_TYPE = 0x8C000000;
	constexpr inline u32 LUI_TYPE = 0x3C000000;

	inline bool _isInstructionType6Bits(u32 const *code, u32 const type) {
		return (code[0] & 0xFC000000) == type;
	}

	inline bool _isInstructionType11Bits(u32 const *code, u32 const type) {
		return (code[0] & 0xFFE00000) == type;
	}

	inline u32 const *_findCtc0T017Forwards(u32 const *code, u32 const maxLength) {
		constexpr u32 ctc0_t0_17_instr = 0x40C88800;

		for(u32 i = 0; i < maxLength; ++i) {
			if (code[i] == ctc0_t0_17_instr) {
				return &code[i];
			}
		}

		return nullptr;
	}

	inline u32 const *_findJalForwards(u32 const *code, u32 const maxLength) {
		for(u32 i = 0; i < maxLength; ++i) {
			if (_isInstructionType6Bits(&code[i], JAL_TYPE)) {
				return &code[i];
			}
		}

		return nullptr;
	}

	inline u32 const *_findJrRaForwards(u32 const *code, u32 const maxLength) {
		constexpr u32 jra_ra_instr = 0x03E00008;

		for(u32 i = 0; i < maxLength; ++i) {
			if (code[i] == jra_ra_instr) {
				return &code[i];
			}
		}

		return nullptr;
	}

	inline u32 const *_findLuiForwards(u32 const *code, u32 const maxLength) {
		for(u32 i = 0; i < maxLength; ++i) {
			if (_isInstructionType11Bits(&code[i], LUI_TYPE)) {
				return &code[i];
			}
		}

		return nullptr;
	}

	inline u32 const *_findFirstNonLwBackwards(u32 const *code, s32 const maxLength) {
		for(s32 i = 0; i >= -maxLength; --i) {
			if (!_isInstructionType6Bits(&code[i], LW_TYPE)) {
				return &code[i];
			}
		}

		return nullptr;
	}

	inline u32 const *_extractBranchAddr(u32 const *code) {
		auto const offset = (code[0] & 0xFFFF);
		return &code[offset+1];
	}

	inline u32 _extractAddiu(u32 const *code) {
		return (code[0] & 0xFFFF);
	}

	inline u32 const *_extractJalAddr(u32 const *code) {
		return reinterpret_cast<u32* const>((code[0] & 0x3FFFFFF) << 2);
	}

	inline u32 _makeJ(void const *addr) {
		return 0x08000000 | ((reinterpret_cast<u32>(addr) & 0x0ffffffc) >> 2);
	}

	inline u32 _makeJal(void const *addr) {
		return 0x0C000000 | ((reinterpret_cast<u32>(addr) & 0x0ffffffc) >> 2);
	}
}

void sha256hmacPatched(u8 const *key, u32 keylen, u8 const *data, u32 datalen, u8 *out) {
	if constexpr (isDebug) {
		printf("0x%x 0x%x 0x%x\n", key, data, out);
	}

	auto const bit32_key = reinterpret_cast<u32 const*>(key);
	auto const bit32_data = reinterpret_cast<u32 const*>(data);

	if constexpr (isDebug) {
		printf("0x%x 0x%x\n", *bit32_key, *bit32_data);
	}

	auto const key_idx = *bit32_key;

	for(u32 i = 0; i < HASH_ROWS; ++i) {
		if (key_idx == g_KeyTable[i].idx) {
			if constexpr (isDebug) {
				printf("Found matching hmac 0x%x\n", g_KeyTable[i].idx);
			}

			memcpy(out, g_KeyTable[i].key, sizeof(KeyEntry::key));

			return;
		}
	}

	if constexpr (isDebug) {
		printf("Key not found, PSP won't boot!\n");
	}
}

void noBootromPatch(void *entryPoint) {
	auto const memory = reinterpret_cast<u32*>(entryPoint);
	auto const offset = reinterpret_cast<u32>(entryPoint);

	//Find "ctc0 $t0, $17"
	auto const ctc0_addr = _findCtc0T017Forwards(memory, 500);

	if constexpr (isDebug) {
		printf("ctc0 at 0x%x\n", ctc0_addr);
	}

	//Not a Sony IPL, abort
	if (ctc0_addr == nullptr) {
		return;
	}

	// auto const branch_addr = _extractBranchAddr(ctc0_addr+1);
	
	// if constexpr (isDebug) {
	// 	printf("branch to 0x%x\n", branch_addr);
	// }

	//Find "lui $t0, 0xBFD0"
	auto const lui_pos = _findLuiForwards(ctc0_addr, 0x10000);

	//Next instruction is "lui $t2, HI(after_reset_addr)"
	auto const second_lui_pos = lui_pos + 1;

	if constexpr (isDebug) {
		printf("second lui at 0x%x\n", second_lui_pos);
	}

	//Next instruction is "addiu $t2, LO(after_reset_addr)"
	//PSP will resume execution from here after CPU reset
	//We are interested in this addr
	auto const ori_pos = second_lui_pos + 1;

	//Extract and reassemble the address
	auto const ori_val = _extractAddiu(ori_pos);
	auto const branch_addr = reinterpret_cast<u32 const*>(offset | ori_val);

	//Find first call that occurs in the binary (will be needed later)
	auto const first_call_addr = _findJalForwards(memory, 0x10000);

	if constexpr (isDebug) {
		printf("func call 0x%x\n", first_call_addr);
	}

	//First universal patch
	//Above "ctc0 $t0, $17" is "lw $t0, 0xFFC($t0)"
	//Replace it with "cfc0, $t0, $17", so it becomes
	//essentially no-op (writes the same value to the register back)
	//This prevents the correct bootrom timestamp 
	//(set by our loader) to be overwritten with gargabe
	auto const patchPoint0 = const_cast<u32*>(ctc0_addr-1);

	constexpr u32 cfc0_t0_17_instr = 0x40488800; //"cfc0, $t0, $17"
	patchPoint0[0] = cfc0_t0_17_instr;

	//Second universal patch (1st version) -- commented out
	//At lui_pos PSP starts preparing for CPU reset
	//Replace it with an unconditional jump
	//to the place where CPU would land after
	//the reset handler (branch_addr)
	auto const patchPoint1 = const_cast<u32*>(lui_pos);

	// if constexpr (isDebug) {
	// 	printf("makeJ 0x%x\n", _makeJ(branch_addr));
	// }

	//patchPoint1[0] = _makeJ(branch_addr);
	//patchPoint1[1] = 0;

	//Second universal patch (2nd version)
	//Replace 0xBFD0 in "lui $t0, 0xBFD0" with 0xBFC0 and just allow PSP to reset
	//constexpr u32 lui_t0_0xBFC0 = 0x3C08BFC0; //lui $t0, 0xBFC0
	patchPoint1[0] = (patchPoint1[0] & 0xFFFF0000) | 0xBFC0;

	//If the first call we found occurs somewhere after "ctc0 $t0, $17",
	//then it's the simplest (oldest) IPL with no crypto. We are done!
	if (ctc0_addr < first_call_addr) {
		if constexpr (isDebug) {
			printf("IPL Type0\n");
		}
		return;
	}

	//Otherwise it's near the beginning of the binary
	//and it's a decryption function (don't know which one
	//yet, there are two possibilities)
	//Extract the function pointer from the call
	auto const unk_decr_addr = _extractJalAddr(first_call_addr);

	if constexpr (isDebug) {
		printf("func call target 0x%x\n", unk_decr_addr);
	}

	//Walk along the function code and find where it ends
	//with "jr $ra"
	auto const jrra_addr = _findJrRaForwards(unk_decr_addr, 0x10000);

	if constexpr (isDebug) {
		printf("jr ra 0x%x\n", jrra_addr);
	}

	//Now walk backwards, through the function epilogue
	//(i.e. the process of restoring previous values of registers), untill
	//an instruction that is not part of the epilogue is found
	auto const nonlw_addr = _findFirstNonLwBackwards(jrra_addr-1, 0x10000);

	if constexpr (isDebug) {
		printf("0x%x 0x%x\n", nonlw_addr, nonlw_addr-1);
	}

	//We look at the second-to-last instruction of the function
	//(before epilogue), and check if it's a function call
	auto const jal_candidate = nonlw_addr-1;

	//If it's not a call, then this is an IPL that uses
	//the older decryption function
	if (!_isInstructionType6Bits(jal_candidate, JAL_TYPE)) {
		if constexpr (isDebug) {
			printf("Type1\n");
		}

		//We were analysing the older decryption function
		auto const decrypt_addr = unk_decr_addr;

		if constexpr (isDebug) {
			printf("decrypt: 0x%x\n", decrypt_addr);
		}

		//Find the first call in this older decryption function
		//It will be a call to sha256hmac function
		auto sha256hmac_addr = _findJalForwards(decrypt_addr, 0x10000);

		if constexpr (isDebug) {
			printf("sha256hmac: 0x%x\n", sha256hmac_addr);
		}

		//First recent IPL patch
		//We replce it with our function that will provide a pre-calculated key
		//IPL no longer needs bootrom data to decrypt stage2!
		auto const patchPoint2 = const_cast<u32*>(sha256hmac_addr);
		patchPoint2[0] = _makeJal(reinterpret_cast<void const *>(sha256hmacBufPatched));
	} else {
		if constexpr (isDebug) {
			printf("Type2\n");
		}
		//It it a call. That means we are in the "new" decryption function,
		//and this call leads to the older decryption function
		auto const decrypt_addr = _extractJalAddr(nonlw_addr-1);

		if constexpr (isDebug) {
			printf("decrypt: 0x%x\n", decrypt_addr);
		}

		//Find the first call in this older decryption function
		//It will be a call to sha256hmacBuf function
		auto sha256hmac_addr = _findJalForwards(decrypt_addr, 0x10000);

		if constexpr (isDebug) {
			printf("sha256hmac: 0x%x\n", sha256hmac_addr);
		}

		//First recent IPL patch
		//We replce it with our function that will provide a pre-calculated key
		auto const patchPoint2 = const_cast<u32*>(sha256hmac_addr);
		patchPoint2[0] = _makeJal(reinterpret_cast<void const *>(sha256hmacBufPatched));

		//Second recent IPL patch -- not needed
		//The "new" decryption function just scrambles the data a bit more before
		//calling the older decryption function which does the actual work
		//Because we provide the pre-computed key, we do not need it,
		//we can call the older decryption function directly
		// auto const patchPoint3 = const_cast<u32*>(first_call_addr);
		// patchPoint3[0] = _makeJal(decrypt_addr);
	}
}
