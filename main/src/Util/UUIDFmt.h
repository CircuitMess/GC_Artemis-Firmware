#ifndef ARTEMIS_FIRMWARE_UUIDFMT_H
#define ARTEMIS_FIRMWARE_UUIDFMT_H

#include <cstdint>
#include <cstdio>
#include <string>

inline std::string formatUUID128(const uint8_t (&u)[16]){
	char buf[37];
	std::snprintf(buf, sizeof(buf),
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		u[15], u[14], u[13], u[12], u[11], u[10], u[9], u[8],
		u[7], u[6], u[5], u[4], u[3], u[2], u[1], u[0]);
	return buf;
}

#endif
