#include <sstream>
#include <iomanip>
#include "Tools.h"

std::string vectorToHexString(const std::vector<uint8_t>& vec) {
	std::ostringstream stream;
	stream << "(" << vec.size() << " bytes): ";
	stream << std::setw(2) << std::setfill('0') << std::hex;
	for(auto it = vec.begin(); it != vec.end() ; it++) {
    	if (it != vec.begin()) {
    		stream << " ";
		}
		stream << static_cast<unsigned int>(*it);
    }
	return stream.str();
}
