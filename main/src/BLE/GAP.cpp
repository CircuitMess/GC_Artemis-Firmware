#include "GAP.h"
#include "Client.h"
#include "Server.h"
#include "ConMan.h"
#include <esp_log.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>

static const char* TAG = "BLE";

BLE::GAP* BLE::GAP::self = nullptr;

BLE::GAP::GAP(){
	self = this;

	esp_ble_gatt_set_local_mtu(500);

	esp_ble_gap_register_callback([](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){
		if(self == nullptr) return;
		self->ble_GAP_cb(event, param);
	});

	esp_ble_gap_set_device_name("Artemis Watch"); // TOD: somewhere else, doesn't update the device name when discovering on phone-
	esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_GENERIC_WATCH);

	initSecure();

	// Usually done when GATTC registers (ESP_GATTC_REG_EVT)
	esp_ble_gap_config_adv_data((esp_ble_adv_data_t*) &AdvertConfig);
	esp_ble_gap_config_adv_data((esp_ble_adv_data_t*) &AdvertRespConfig);
}

BLE::GAP::~GAP(){
	self = nullptr;
	esp_ble_gap_register_callback([](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){});
}

void BLE::GAP::setClient(Client* client){
	this->client = client;
}

void BLE::GAP::setServer(Server* server){
	this->server = server;
}

void BLE::GAP::startAdvertising(){
	esp_ble_gap_start_advertising((esp_ble_adv_params_t*) &AdvertParams);
}

void BLE::GAP::ble_GAP_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param){
	ESP_LOGV(TAG, "GAP_EVT, event %d", event);
	// TODO: handle ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT -> contains connection parameters (min & max interval, etc.)

	switch(event){
		case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
			ConMan.confDone(param->update_conn_params.status == ESP_BT_STATUS_SUCCESS);
			break;

		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			configDone(Config::ScanResponse);
			break;

		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			configDone(Config::Advertising);
			break;

		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
			//advertising start complete event to indicate advertising start successfully or failed
			if(param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS){
				ESP_LOGE(TAG, "advertising start failed, error status = %x", param->adv_start_cmpl.status);
				break;
			}
			ESP_LOGI(TAG, "advertising start success");
			break;

		case ESP_GAP_BLE_NC_REQ_EVT:
			/* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
			show the passkey number to the user to confirm it with the number displayed by peer device. */
			esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true); // true for accept, false for not accept
			ESP_LOGI(TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%lu", param->ble_security.key_notif.passkey);
			break;

		case ESP_GAP_BLE_SEC_REQ_EVT:
			/* send the positive(true) security response to the peer device to accept the security request.
			If not accept the security request, should send the security response with negative(false) accept value*/
			esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
			break;

		case ESP_GAP_BLE_AUTH_CMPL_EVT:{
			if(!param->ble_security.auth_cmpl.success){
				ESP_LOGI(TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
				break;
			}
			ESP_LOGI(TAG, "paired");
			esp_log_buffer_hex("addr", param->ble_security.auth_cmpl.bd_addr, ESP_BD_ADDR_LEN);

			if(client){
				client->onPairDone();
			}

			if(server){
				server->onPairDone();
			}

			break;
		}

		default:
			break;
	}
}

void BLE::GAP::configDone(Config config){
	configsDone.insert(config);
	if(configsDone.size() == (int) Config::COUNT){
		ConMan.disconnect(); // Starts advertising
	}
}

void BLE::GAP::initSecure(){
	uint32_t passkey = 123456; // static passkey
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));

	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND; // bonding with peer device after authentication
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

	esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE; // set the IO capability to No output No input
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

	uint8_t key_size = 16; // the key size should be 7~16 bytes
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));

	uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));

	uint8_t oob_support = ESP_BLE_OOB_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));

	/* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
	and the response key means which key you can distribute to the master;
	If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
	and the init key means which key you can distribute to the slave. */

	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));

	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}
