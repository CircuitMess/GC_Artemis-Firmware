#include "HWVersion.h"

bool HWVersion::check(){
	readVersion(CachedVersion);
	if(CachedVersion == Version){
		return true;
	}

	log();

	return false;
}

bool HWVersion::write(){
	esp_err_t err = esp_efuse_batch_write_begin();
	if(err != ESP_OK){
		return false;
	}

	err = esp_efuse_write_field_blob((const esp_efuse_desc_t**) Efuse_ver, &Version, 16);
	if(err != ESP_OK){
		return false;
	}

	err = esp_efuse_batch_write_commit();
	if(err != ESP_OK){
		return false;
	}

	return true;
}

void HWVersion::log(){
	ESP_LOGE("Hardware check", "Hardware version (0x%04x) does not match software version (0x%04x).", CachedVersion, Version);
}

bool HWVersion::readVersion(uint16_t& version){
	const esp_err_t err = esp_efuse_read_field_blob((const esp_efuse_desc_t**) Efuse_ver, &version, 16);
	if(err != ESP_OK){
		return false;
	}

	return true;
}

uint16_t HWVersion::getHardcodedVersion(){
	return Version;
}
