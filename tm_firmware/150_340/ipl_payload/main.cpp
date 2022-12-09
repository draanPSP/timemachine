#include <string.h>

#include <pspmacro.h>
#include <type_traits>

#include <tm_common.h>
#include <payloadex.h>

namespace {
	using v_v_function_t = std::add_pointer_t<void()>;

	inline auto const cache1Ptr = reinterpret_cast<v_v_function_t const>(0x0400011c);
	inline auto const cache2Ptr = reinterpret_cast<v_v_function_t const>(0x04000c78);
	inline auto const mainEntryPtr = reinterpret_cast<v_v_function_t const>(0x04000000);

	[[noreturn]] inline void mainEntry() {

		asm("li	$sp, 0x040FFF00");
		mainEntryPtr();

		__builtin_unreachable();
	}

	void clearCaches() {
		cache1Ptr();
		cache2Ptr();
	}

	void loadPayloadEx() {

		//Copy payloadex to 0x88700000
		memcpy((void*)0x08700000, payloadex, sizeof(payloadex));

		clearCaches();		
	}
}

int main() {

	MAKE_CALL(0x04000068, loadPayloadEx);

	//Change payload entry point to payloadex
	_sw(0x3c198870, 0x0400a758);

	//Avoid TA-082 clockgen incompatibility 
	_sw(0x1000ff07, 0x04000858);

	clearCaches();

	mainEntry();
}