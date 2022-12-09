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

	inline auto const mainEntryPtr = reinterpret_cast<v_v_function_t const>(0x04000000);

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
}

 int main() {

	MAKE_CALL(0x04000364, loadPayloadEx);

	//Change payload entry point to payloadex
	_sw(LUI(GPREG_T9, 0x88fc), LD_PAYLOADEX_ADDR);

#ifdef SET_SEED_ADDRESS
	MAKE_CALL(SET_SEED_ADDRESS, setSeed);
#endif

	clearCaches();

	mainEntry();
}