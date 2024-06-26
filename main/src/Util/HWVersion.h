#ifndef ARTEMIS_FIRMWARE_HWVERSION_H
#define ARTEMIS_FIRMWARE_HWVERSION_H

#include <cstdint>
#include <esp_efuse.h>

class HWVersion {
public:
	static bool check();
	static bool write();
	static void log();

private:
	static inline uint16_t CachedVersion = 0;
	static inline constexpr const uint16_t Version = 0x0003;
	static constexpr esp_efuse_desc_t Ver = { EFUSE_BLK3, 16, 16 };
	static constexpr const esp_efuse_desc_t* Efuse_ver[] = { &Ver, nullptr };
};

#endif //ARTEMIS_FIRMWARE_HWVERSION_H