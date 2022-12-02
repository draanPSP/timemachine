#include <string.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>

#if PSP_MODEL == 0
#include <payloadex_01g.h>
#define LD_PAYLOADEX_ADDR   0x0400cf18
#elif PSP_MODEL == 1
#include <payloadex_02g.h>
#define LD_PAYLOADEX_ADDR   0x0400dc98
#endif

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;
	using syscon_read_t = std::add_pointer_t<void(u32,s32)>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x040004dc);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x040004f8);
	inline auto const mainEntryPtr = reinterpret_cast<v_v_function_t const>(0x04000000);

#if PSP_MODEL == 1
	inline auto const seedRetPtr = reinterpret_cast<v_v_function_t const>(0x040011f4);
#endif

	[[noreturn]] inline void mainEntry() {

		asm("li	$sp, 0x040fff00");
		mainEntryPtr();

		__builtin_unreachable();
	}

	void clearCaches() {
		cache1Ptr();
		cache2Ptr();
	}

	void loadPayloadEx() {

		//Copy payloadex to 0x88fc0000
		memcpy(reinterpret_cast<void*>(0x88fc0000), payloadex, sizeof(payloadex));

		clearCaches();
	}

#if PSP_MODEL == 1

	const unsigned int seedKey[] = {
		0x830D9389, 0xF2027F71, 0x2D060DC5, 0xBA78A905,
		0x26C9824D, 0x3335A7A5, 0xD5CD6368, 0xD136A11D,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};

	void seedPatch() {

		memcpy(reinterpret_cast<void*>(0xBFC00200), seedKey, sizeof(seedKey));
		seedRetPtr();
	}

#endif
}

int main() {

	MAKE_CALL(0x04000364, loadPayloadEx);

	//Change payload entry point to payloadex
	_sw(LUI(GPREG_T9, 0x88fc), LD_PAYLOADEX_ADDR);

#if PSP_MODEL == 1
	REDIRECT_FUNCTION(0x04001170, seedPatch);
#endif

	clearCaches();

	mainEntry();
}