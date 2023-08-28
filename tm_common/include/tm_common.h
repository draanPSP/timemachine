#ifndef TM_COMMON
#define TM_COMMON

#include <hw.h>

constexpr inline std::uint32_t TM_MAX_PATH_LENGTH = 64;
constexpr inline std::uint32_t TM_CONFIG_SECTOR = 1;

constexpr inline bool isDebug = false;

//Store the model number in a temporary memory location
//that won't be overwritten very soon, for example
//last dword of the scratchpad memory 
inline void tmTempSetModel(std::uint32_t const model) {
	memoryK0(0x00013FFC) = model;
}

//Fetch the model number from the temporary memory location
inline std::uint32_t tmTempGetModel() {
	return memoryK0(0x00013FFC);
}

//Store the information if the bootrom access is lost or not
inline void tmTempSetNoBootrom(bool const flag) {
	memoryK0(0x00013FF8) = flag;
}

//Fetch the information if the bootrom access is lost or not
inline bool tmTempIsNoBootrom() {
	return memoryK0(0x00013FF8);
}

#endif //TM_COMMON
