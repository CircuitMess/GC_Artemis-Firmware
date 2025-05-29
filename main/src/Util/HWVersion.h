#ifndef ARTEMIS_FIRMWARE_HWVERSION_H
#define ARTEMIS_FIRMWARE_HWVERSION_H

#include <cstdint>
#include <esp_efuse.h>

#if CONFIG_VERSION != 0x0003 && CONFIG_VERSION != 0x0007
	#error "Only 0x0003 or 0x0007 hardware versions are supported."
#endif

class HWVersion {
public:
	static bool check();
	static bool write();
	static void log();

	static bool readVersion(uint16_t &version);

	static uint16_t getHardcodedVersion();

private:
	static inline uint16_t CachedVersion = 0;
	static inline constexpr const uint16_t Version = CONFIG_VERSION;
	static constexpr esp_efuse_desc_t Ver = { EFUSE_BLK3, 16, 16 };
	static constexpr const esp_efuse_desc_t* Efuse_ver[] = { &Ver, nullptr };
};

#endif //ARTEMIS_FIRMWARE_HWVERSION_H