#include <sstream>
#include <iomanip>
#include "Tools.h"

/**
 * @brief Get bits 7 to 4 of a 8-bit value (the high nibble)
 *
 * @param byte The 8-bit input value
 * @return Bits 7 to 4 extracted and shifted to buts 3 to 0 as a byte (uint8_t)
 */
static inline uint8_t u8_get_hi_nibble(const uint8_t byte) {
	return static_cast<uint8_t>(byte >> 4);
}

/**
 * @brief Get bits 3 to 0 of a 8-bit value (the low nibble)
 *
 * @param byte The 8-bit input value
 * @return Bits 3 to 0 extracted as a byte (uint8_t)
 */
static inline uint8_t u8_get_lo_nibble(const uint8_t byte) {
	return static_cast<uint8_t>(byte & 0x0f);
}

/**
 * @brief Convert a byte to its 2-digit hexadecimal representation
 *
 * @param byte The byte to represent
 * @return A 2-character string contaning the hexadecimal representation of @p byte
 */
static inline std::string byteToHexString(uint8_t byte) {
	std::string result("00");
	uint8_t nibble = u8_get_hi_nibble(byte);
	if (nibble<=9) {
		result[0] = nibble + '0';
	}
	else {
		result[0] = nibble - 0x0a + 'a';
	}
	nibble = u8_get_lo_nibble(byte);
	if (nibble<=9) {
		result[1] = nibble + '0';
	}
	else {
		result[1] = nibble - 0x0a + 'a';
	}
	return result;
}

/**
 * @brief Serialize a std::vector to an iostream
 *
 * @param out The original output stream
 * @param data The object to serialize
 *
 * @return The new output stream with serialized data appended
 */
std::string vectorToHexString(const std::vector<uint8_t>& vec) {
	std::ostringstream out;
	out << "(" << vec.size() << " bytes): ";
		for (auto it=std::begin(vec); it<std::end(vec); it++) {
		if (it != std::begin(vec)) {
			out << std::string(" ");
		}
		out << byteToHexString(*it);
	}
	return out.str();
}
