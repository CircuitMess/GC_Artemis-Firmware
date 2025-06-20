#include "EfuseMeta.h"

bool EfuseMeta::check(){
	readPID(CachedPID);
	readRev(CachedRevision);

	//Make an exception for this product a having blank PID!
	if(CachedPID == PID || CachedPID == 0){
		return true;
	}

	log();

	return false;
}

bool EfuseMeta::write(){
	esp_err_t err = esp_efuse_batch_write_begin();
	if(err != ESP_OK){
		return false;
	}

	err = esp_efuse_write_field_blob((const esp_efuse_desc_t**) PID_Blob, &PID, 16);
	if(err != ESP_OK){
		return false;
	}

	err = esp_efuse_write_field_blob((const esp_efuse_desc_t**) Rev_Blob, &HWRevision, 8);
	if(err != ESP_OK){
		return false;
	}


	err = esp_efuse_batch_write_commit();
	if(err != ESP_OK){
		return false;
	}

	return true;
}

void EfuseMeta::log(){
	ESP_LOGE("Hardware check", "PID (0x%04x), rev(0x%02x) does not match software (0x%04x).", CachedPID, CachedRevision, PID);
}

bool EfuseMeta::readPID(uint16_t& pid){
	const esp_err_t err = esp_efuse_read_field_blob((const esp_efuse_desc_t**) PID_Blob, &pid, 16);
	if(err != ESP_OK){
		return false;
	}

	return true;
}

bool EfuseMeta::readRev(uint8_t& Revision){
	const esp_err_t err = esp_efuse_read_field_blob((const esp_efuse_desc_t**) Rev_Blob, &Revision, 8);
	if(err != ESP_OK){
		return false;
	}

	return true;
}

uint16_t EfuseMeta::getHardcodedPID(){
	return PID;
}

uint16_t EfuseMeta::getHardcodedRevision(){
	return HWRevision;
}
