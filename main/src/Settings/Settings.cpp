#include "Settings.h"
#include <nvs_flash.h>
#include <esp_log.h>

static const char* TAG = "Settings";

Settings::Settings(){
	auto err = nvs_open(NVSNamespace, NVS_READWRITE, &handle);
	ESP_ERROR_CHECK(err);
	load();
}

SettingsStruct Settings::get(){
	return settingsStruct;
}

void Settings::set(SettingsStruct& settings){
	settingsStruct = settings;
}

void Settings::store(){
	esp_err_t err = nvs_set_blob(handle, BlobName, &settingsStruct, sizeof(SettingsStruct) - sizeof(ThemeStruct) + sizeof(Theme));

	if(err != ESP_OK){
		ESP_LOGW(TAG, "NVS settings store error: %d", err);
		return;
	}

	err = nvs_commit(handle);
	if(err != ESP_OK){
		ESP_LOGW(TAG, "NVS settings commit error: %d", err);
	}
}

void Settings::load(){
	size_t out_size = sizeof(SettingsStruct) - sizeof(ThemeStruct) + sizeof(Theme);

	auto err = nvs_get_blob(handle, BlobName, &settingsStruct, &out_size);

	if(err != ESP_OK){
		ESP_LOGI(TAG, "Settings not found, writing defaults");
		store();
		err = nvs_get_blob(handle, BlobName, &settingsStruct, &out_size);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Couldn't access NVS settings: %d", err);
		}
	}

	settingsStruct.themeData = createTheme(settingsStruct.themeData.theme);
}
