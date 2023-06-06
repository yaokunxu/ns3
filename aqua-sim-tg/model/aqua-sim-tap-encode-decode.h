/*
 * aqua-sim-tap-encode-decode.h
 *
 *  Created on: Feb 26, 2020
 *      Author: anna
 */

#ifndef AQUA_SIM_TAP_ENCODE_DECODE_H_
#define AQUA_SIM_TAP_ENCODE_DECODE_H_


#include <string>

namespace ns3 {

	std::string TapBufferToString(uint8_t* buffer, uint32_t len);
	bool TapStringToBuffer(std::string s, uint8_t* buffer, uint32_t* len);


} // namespace ns3



#endif /* AQUA_SIM_TAP_ENCODE_DECODE_H_ */
