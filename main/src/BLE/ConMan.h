#ifndef CLOCKSTAR_FIRMWARE_CONMAN_H
#define CLOCKSTAR_FIRMWARE_CONMAN_H

#include "GAP.h"
#include "ConConf.h"
#include <esp_gap_ble_api.h>

class ConManager {
public:

	// Thread contract: all public methods must be invoked from the BLE event task
	// (the bluedroid BTC task that drives GAP/GATTS callbacks). The internal state
	// (`connected`, `advertising`, `wantAdv`, `lowPow`, `conConf`) is unsynchronized and
	// relies on this invariant.
	void connect(const esp_bd_addr_t addr);
	void disconnect();

	// Called once the GAP config (adv/scan-response data) is ready, to kick off advertising.
	void start();

	void goLowPow();
	void goHiPow();

private:
	friend BLE::GAP;
	void confDone(bool success);
	ConConf conConf;

	bool connected = false;
	esp_bd_addr_t current;

	// `advertising` is the host-side hint for whether the radio is currently advertising;
	// it is *not* a faithful mirror of bluedroid's internal state. It is set on
	// ADV_START/ADV_STOP complete events, and additionally cleared in connect() to cover
	// bluedroid's implicit advertising stop on a connection event (which does not always
	// surface an ADV_STOP_COMPLETE_EVT). `wantAdv` is the requested intent set by
	// start/stopAdv. startAdv() / stopAdv() short-circuit when `advertising` already
	// matches intent, so this hint must stay synchronized at every implicit-state-change
	// site.
	bool advertising = false;
	bool wantAdv = false;

	bool lowPow = false;

	void startAdv();
	void stopAdv();
	void setCon();

	// Fired by GAP from ADV_START/ADV_STOP complete events on the BLE/BTC task.
	void onAdvStartComplete(bool success);
	void onAdvStopComplete(bool success);

	static constexpr esp_ble_adv_params_t AdvLowPow = {
			.adv_int_min        = 2056, // 1285ms = 2056 * 0.625ms
			.adv_int_max        = 2056, // Apple: Interval should be 20ms. After 30 seconds of no connection, feel free to switch to 1285ms
			.adv_type           = ADV_TYPE_IND,
			.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
			.channel_map        = ADV_CHNL_ALL,
			.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
	};

	static constexpr esp_ble_adv_params_t AdvHiPow = {
			.adv_int_min        = 32, // 20ms = 32 * 0.625ms
			.adv_int_max        = 32,
			.adv_type           = ADV_TYPE_IND,
			.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
			.channel_map        = ADV_CHNL_ALL,
			.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
	};

	static constexpr esp_ble_conn_update_params_t ConLowPow = {
			.min_int = 720, // min_int = 720*1.25ms = 900ms
			.max_int = 960, // max_int = 960*1.25ms = 1125ms
			.latency = 0,
			.timeout = 600 // timeout = 500*10ms = 5000ms
	};

	static constexpr esp_ble_conn_update_params_t ConHiPow = {
			.min_int = 24, // min_int = 24*1.25ms = 30ms
			.max_int = 40, // max_int = 40*1.25ms = 50ms
			.latency = 0,
			.timeout = 400 // timeout = 500*10ms = 5000ms
	};

};

extern ConManager ConMan;

#endif //CLOCKSTAR_FIRMWARE_CONMAN_H
