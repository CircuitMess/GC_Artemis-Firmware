#ifndef ARTEMIS_FIRMWARE_HWVERSION_H
#define ARTEMIS_FIRMWARE_HWVERSION_H

#include <cstdint>
#include <esp_efuse.h>

/**
 * Used for reading/writing product IDs, revision numbers from efuse.
 * (previously called HWVersion)
 */
class EfuseMeta {
public:
	static bool check();
	static bool write();
	static void log();

	static bool readPID(uint16_t& pid);
	static bool readRev(uint8_t& Rev);

	static uint16_t getHardcodedPID();
	static uint16_t getHardcodedRevision();

private:
	static inline uint16_t CachedPID = 0;
	static inline constexpr const uint16_t PID = 0x0003;

	static inline constexpr const uint8_t HWRevision = 2;
	static inline uint8_t CachedRevision = 0;

	static constexpr esp_efuse_desc_t PIDBlock = { EFUSE_BLK3, 16, 16 };
	static constexpr const esp_efuse_desc_t* PID_Blob[] = { &PIDBlock, nullptr };
	static constexpr esp_efuse_desc_t RevBlock = { EFUSE_BLK3, 32, 8 };
	static constexpr const esp_efuse_desc_t* Rev_Blob[] = { &RevBlock, nullptr };
};

#endif //ARTEMIS_FIRMWARE_HWVERSION_H