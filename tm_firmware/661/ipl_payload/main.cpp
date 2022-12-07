/* 
 * Based on IPL patches from 6.61 DC-M33 - https://github.com/balika011/DC-M33/
*/

#include <string.h>
#include <type_traits>

#include <syscon.h>
#include <cache.h>
#include <lowio.h>
#include <pspmacro.h>

#include <tm_common.h>

#if PSP_MODEL == 0
#include <payloadex_01g.h>
#define LD_PAYLOADEX_ADDR   0x0400cf18
#elif PSP_MODEL == 1
#include <payloadex_02g.h>
#define LD_PAYLOADEX_ADDR   0x0400dc98
#define SET_SEED_ADDRESS    0x04001160
#elif PSP_MODEL == 2
#include <payloadex_03g.h>
#define LD_PAYLOADEX_ADDR   0x0400dc98
#define SET_SEED_ADDRESS    0x04001194
#endif

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;

	inline auto const loaderEntryPtr = reinterpret_cast<v_v_function_t const>(0x040ec000);
	inline auto const mainEntryPtr = reinterpret_cast<v_v_function_t const>(0x04000000);

	[[noreturn]] inline void loaderEntry() {

		loaderEntryPtr();

		__builtin_unreachable();
	}

	[[noreturn]] inline void mainEntry() {

		asm("li	$sp, 0x040fff00");
		mainEntryPtr();

		__builtin_unreachable();
	}

	void clearCaches() {
        iplKernelDcacheWritebackInvalidateAll();
		iplKernelIcacheInvalidateAll();
	}

	void loadPayloadEx() {

		//Copy payloadex to 0x88fc0000
		memcpy(reinterpret_cast<void*>(0x88fc0000), payloadex, sizeof(payloadex));

		clearCaches();
	}

#ifdef SET_SEED_ADDRESS
    u8 seed_xor[] =
	{
#if PSP_MODEL == 1
		0x28, 0x0e, 0xf4, 0x16, 0x45, 0x59, 0xfe, 0x8c, 0xa6, 0x58, 0x25, 0x51, 0xd7, 0x3b, 0x31, 0x4b
#elif PSP_MODEL == 2
		0x42, 0x75, 0x4c, 0xc6, 0xb6, 0xea, 0xfe, 0xa1, 0xeb, 0x60, 0x77, 0xde, 0xa6, 0xc0, 0x37, 0xa0
#endif
	};

	int setSeed(u8 *xor_key, u8 *random_key, u8 *random_key_dec_resp_dec) {

		for (int i = 0; i < sizeof(seed_xor); i++)
			*(u8 *) (0xbfc00210 + i) ^= seed_xor[i];

		return 0;
	}

#endif

	u8 rom_hmac[] =
	{
#if PSP_MODEL == 0
        0x27, 0x85, 0xcf, 0x5c, 0xbb, 0xad, 0xf4, 0x2f, 0xff, 0xa5, 0xc4, 0x90, 0xb3, 0xa0, 0xa5, 0x64,
        0xd9, 0x29, 0xdb, 0xe2, 0xdb, 0x07, 0x35, 0x4a, 0xe7, 0x76, 0xcf, 0x51, 0x00, 0x00, 0x00, 0x00
#elif PSP_MODEL == 1
        0xee, 0x38, 0xdb, 0x54, 0xa2, 0xae, 0xcf, 0x84, 0xfb, 0x25, 0x59, 0xf8, 0xa5, 0x99, 0xa0, 0xcb,
        0xab, 0xdf, 0x92, 0x92, 0x39, 0x9b, 0xa5, 0xff, 0x07, 0x15, 0xb3, 0x5d, 0x00, 0x00, 0x00, 0x00
#elif PSP_MODEL == 2
        0x48, 0xf5, 0x2f, 0x88, 0x5f, 0xbe, 0x24, 0x3a, 0xd3, 0x19, 0x01, 0x85, 0x20, 0x42, 0xb0, 0x0b,
        0x63, 0x2a, 0x8f, 0x55, 0x17, 0xa5, 0x9d, 0x61, 0xf1, 0xac, 0x0e, 0x15, 0x00, 0x00, 0x00, 0x00
#endif
	};

	void sha256hmacPatch(u8 *key, u32 keylen, u8 *data, u32 datalen, u8 *out) {
		memcpy(reinterpret_cast<void*>(out), rom_hmac, keylen);
	}

    void patchMainBin() {

        MAKE_CALL(0x04000364, loadPayloadEx);

        //Change payload entry point to payloadex
	    _sw(LUI(GPREG_T9, 0x88fc), LD_PAYLOADEX_ADDR);

#ifdef SET_SEED_ADDRESS
        MAKE_CALL(SET_SEED_ADDRESS, setSeed);
#endif

        clearCaches();

        mainEntry();
    }
}

int main() {

    iplSysconInit();

	bool isPreIplMapped = true;
	auto const tachyonVer = iplSysregGetTachyonVersion();
	u32 timestamp = sdkGetBootromTimestampFromRom();

	if (tachyonVer < 0x400000 && timestamp != 0x20040420) {
		_sw(0x20040420, 0xbfc00ffc);
		isPreIplMapped = false;
	}
	else if (tachyonVer >= 0x400000 && tachyonVer < 0x600000 && timestamp != 0x20050104) {
		_sw(0x20050104, 0xbfc00ffc);
		isPreIplMapped = false;
	}
	else if (tachyonVer >= 0x600000 && timestamp != 0x20070910) {
		_sw(0x20070910, 0xbfc00ffc);
		isPreIplMapped = false;
	}

	//NOP out memset of 0x040e0000-0x040ec000
	_sw(0, 0x40ec0d8);

	//If IPL is not mapped 0xbfd00000 has been remapped to 0xbfc00000
	if (!isPreIplMapped)
		_sw(0x3c08bfc0, 0x40ec12c);

    MAKE_CALL(0x040ed000, sha256hmacPatch);

    MAKE_JUMP(0x040ec2c8,  patchMainBin);

    clearCaches();

    loaderEntry();
}