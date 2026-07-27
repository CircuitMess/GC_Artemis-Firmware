#include "ConMan.h"
#include <cstring>
#include <esp_log.h>

static const char* TAG = "BLE::ConMan";

ConManager ConMan;

void ConManager::confDone(bool success){
	conConf.confDone(success);
}

void ConManager::start(){
	startAdv();
}

void ConManager::connect(const esp_bd_addr_t addr){
	// Bluedroid implicitly stops advertising on a connection and does not always fire
	// ADV_STOP_COMPLETE_EVT for it. Sync the host-side hint here so the startAdv() /
	// stopAdv() short-circuits below match the real radio state.
	advertising = false;

	connected = true;
	memcpy(current, addr, 6);
	setCon();

	stopAdv();
}

void ConManager::disconnect(){
	connected = false;
	conConf.reset();
	startAdv();
}

void ConManager::goLowPow(){
	lowPow = true;
	if(connected){
		setCon();
	}else{
		startAdv();
	}
}

void ConManager::goHiPow(){
	lowPow = false;
	if(connected){
		setCon();
	}else{
		startAdv();
	}
}

void ConManager::startAdv(){
	wantAdv = true;
	if(advertising) return;
	auto err = esp_ble_gap_start_advertising((esp_ble_adv_params_t*) (lowPow ? &AdvLowPow : &AdvHiPow));
	if(err != ESP_OK){
		ESP_LOGW(TAG, "esp_ble_gap_start_advertising failed: 0x%x", err);
	}
}

void ConManager::stopAdv(){
	wantAdv = false;
	if(!advertising) return;
	auto err = esp_ble_gap_stop_advertising();
	if(err != ESP_OK){
		ESP_LOGW(TAG, "esp_ble_gap_stop_advertising failed: 0x%x", err);
	}
}

void ConManager::onAdvStartComplete(bool success){
	if(!success){
		advertising = false;
		// Honest divergence: GAP rejected our start. Don't auto-retry here — the caller's next
		// state change (next connect/disconnect/goLowPow/goHiPow) will issue another start.
		return;
	}
	advertising = true;

	// If our intent has flipped to "stop" between issuing the start and the event arriving,
	// pull the trigger now to converge.
	if(!wantAdv){
		auto err = esp_ble_gap_stop_advertising();
		if(err != ESP_OK){
			ESP_LOGW(TAG, "esp_ble_gap_stop_advertising failed: 0x%x", err);
		}
	}
}

void ConManager::onAdvStopComplete(bool success){
	if(success){
		advertising = false;
	}

	// If our intent flipped to "start" between issuing the stop and the event arriving, resync.
	if(wantAdv && !advertising){
		startAdv();
	}
}

void ConManager::setCon(){
	/* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
	esp_ble_conn_update_params_t params = {};
	memcpy(&params, lowPow ? &ConLowPow : &ConHiPow, sizeof(esp_ble_conn_update_params_t));
	memcpy(params.bda, current, 6);
	conConf.conf(params);
}
