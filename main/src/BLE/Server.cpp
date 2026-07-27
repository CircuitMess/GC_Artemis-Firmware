#include "Server.h"
#include "GAP.h"
#include "ConMan.h"
#include <esp_log.h>
#include <esp_gatts_api.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "BLE::Server";

BLE::Server* BLE::Server::self = nullptr;

BLE::Server::Server(GAP* gap) : gap(gap){
	if(self != nullptr){
		ESP_LOGE(TAG, "Server already exists");
		return;
	}
	self = this;

	esp_ble_gatts_register_callback([](esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param){
		if(self == nullptr) return;
		self->ble_GATTS_cb(event, gattc_if, param);
	});

	// TODO: This is only needed so GAP can notify the GATT Server when pairing is done
	gap->setServer(this);
}

BLE::Server::~Server(){
	self = nullptr;
}

std::shared_ptr<BLE::Server::Service> BLE::Server::addService(esp_bt_uuid_t uuid){
	esp_gatt_srvc_id_t id = {
			.id = { .uuid = uuid, .inst_id = 0 }, // Assumes only single inst_id
			.is_primary = true
	};

	std::shared_ptr<Service> srv(new Service(id));
	services.insert(srv);
	return srv;
}

void BLE::Server::start(){
	if(iface.appID != 0xff) return;
	iface.appID = AppID;
	esp_ble_gatts_app_register(AppID);
}

BLE::Server::SubHandle BLE::Server::addOnConnectCb(BLE::Server::ConnectCB cb){
	std::lock_guard lock(cbMut);
	const SubHandle handle = nextSubHandle++;
	onConnectCBs.emplace(handle, std::move(cb));
	return handle;
}

BLE::Server::SubHandle BLE::Server::addOnDisconnectCb(BLE::Server::DisconnectCB cb){
	std::lock_guard lock(cbMut);
	const SubHandle handle = nextSubHandle++;
	onDisconnectCBs.emplace(handle, std::move(cb));
	return handle;
}

void BLE::Server::removeOnConnectCb(BLE::Server::SubHandle handle){
	std::lock_guard lock(cbMut);
	onConnectCBs.erase(handle);
}

void BLE::Server::removeOnDisconnectCb(BLE::Server::SubHandle handle){
	std::lock_guard lock(cbMut);
	onDisconnectCBs.erase(handle);
}

void BLE::Server::onPairDone(){
	ESP_LOGI(TAG, "Paired\n");
}

void BLE::Server::ble_GATTS_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param){
	/* 0xff, not specify a certain gatt_if, need to call every profile cb function */
	if(gattc_if == 0xff){
		printf("GATTC CB interface 0xff - event %d\n", event);
	}

	if(event == ESP_GATTS_REG_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_REG_EVT");

		if(param->reg.app_id != iface.appID){
			ESP_LOGE(TAG, "CB: App ID missmatch. Received %d, have %d. Status is %d", param->reg.app_id, iface.appID, param->reg.status);
			return;
		}

		if(param->reg.status == ESP_GATT_OK){
			ESP_LOGI(TAG, "App registered");
			iface.hndl = gattc_if;
		}else{
			ESP_LOGW(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
			iface = {};
			return;
		}

		registerServices();
	}else if(event == ESP_GATTS_CREATE_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_CREATE_EVT");
		onServiceCreated(&param->create);
	}else if(event == ESP_GATTS_START_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_CREATE_EVT");
		if(param->start.status != ESP_GATT_OK){
			ESP_LOGE(TAG, "failed starting, status = 0x%x", param->start.status);
		}
	}else if(event == ESP_GATTS_ADD_CHAR_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_ADD_CHAR_EVT");
		onCharCreated(&param->add_char);
	}else if(event == ESP_GATTS_ADD_CHAR_DESCR_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_ADD_CHAR_DESCR_EVT");
		onCharDescrCreated(&param->add_char_descr);
	}else if(event == ESP_GATTS_MTU_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT");
		onMtuResp(&param->mtu);
	}else if(event == ESP_GATTS_CONNECT_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT");
		onConnect(&param->connect);
	}else if(event == ESP_GATTS_DISCONNECT_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT");
		onDisconnect(&param->disconnect);
	}else{
		switch(event){
			case ESP_GATTS_READ_EVT:
			case ESP_GATTS_WRITE_EVT:
			case ESP_GATTS_EXEC_WRITE_EVT:
				passToChar(event, param);
				break;
			default:
				ESP_LOGV(TAG, "Unhandled event: %d", event);
				break;
		}
	}
}

void BLE::Server::registerServices(){
	for(const auto& service : services){
		esp_ble_gatts_create_service(iface.hndl, &service->id, 1 + service->chars.size()*3);
	}
}

void BLE::Server::onServiceCreated(const esp_ble_gatts_cb_param_t::gatts_create_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "service creation failed, error status = 0x%x", param->status);
		return;
	}

	auto it = std::find_if(services.cbegin(), services.cend(), [param](const std::shared_ptr<Service>& service){
		const auto len = std::min(param->service_id.id.uuid.len, service->id.id.uuid.len);
		return memcmp(param->service_id.id.uuid.uuid.uuid128, service->id.id.uuid.uuid.uuid128, len) == 0;
	});
	if(it == services.end()){
		ESP_LOGW(TAG, "got handle for non-existant service");
		return;
	}
	auto service = *it;

	service->establish(param->service_handle);
}

void BLE::Server::onCharCreated(const esp_ble_gatts_cb_param_t::gatts_add_char_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "char creation failed, error status = 0x%x", param->status);
		return;
	}

	auto it = std::find_if(services.cbegin(), services.cend(), [param](const auto& service){ return param->service_handle == service->hndl; });
	if(it == services.end()){
		ESP_LOGW(TAG, "got handle for non-existant service");
		return;
	}
	auto service = *it;

	auto chr = service->charCreated(param->status, param->char_uuid, std::make_unique<BLE::Server::CharInfo>(this, service.get(), param->attr_handle));
	chars.insert(std::make_pair(param->attr_handle, chr.get()));
}

void BLE::Server::onCharDescrCreated(const esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param* param){
	// We always have exactly one esp_ble_gatts_add_char_descr in flight (see the
	// pendingDescrs comment in Server.h). The head of pendingDescrs is the request
	// being answered by this event, regardless of the descriptor UUID.
	descrInFlight = false;

	if(pendingDescrs.empty()){
		ESP_LOGW(TAG, "ADD_CHAR_DESCR_EVT with no pending request");
		return;
	}

	const auto req = pendingDescrs.front();
	pendingDescrs.pop();

	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "failed adding char descr, status = 0x%x", param->status);
		tryIssueNextDescr();
		return;
	}

	auto it = chars.find(req.charHndl);
	if(it == chars.end()){
		ESP_LOGW(TAG, "Pending descr's parent char hndl %d not registered", req.charHndl);
		tryIssueNextDescr();
		return;
	}

	it->second->ctrlDescrHndl = param->attr_handle;
	chars.insert(std::make_pair(param->attr_handle, it->second));

	ESP_LOGI(TAG, "Added descriptor to characteristic");

	tryIssueNextDescr();
}

void BLE::Server::queueDescr(DescrReq req){
	pendingDescrs.push(req);
	tryIssueNextDescr();
}

void BLE::Server::tryIssueNextDescr(){
	if(descrInFlight || pendingDescrs.empty()) return;

	descrInFlight = true;
	auto& req = pendingDescrs.front();
	esp_ble_gatts_add_char_descr(req.serviceHndl, &req.uuid, req.perm, nullptr, nullptr);
}

void BLE::Server::onMtuResp(const esp_ble_gatts_cb_param_t::gatts_mtu_evt_param* param){
	if(param->conn_id != con.hndl){
		ESP_LOGW(TAG, "Got MTU evt for other con. This: %d, event: %d", con.hndl, param->conn_id);
		return;
	}

	con.MTU_size = param->mtu;
	ESP_LOGI(TAG, "Got MTU event. New MTU is %d B", param->mtu);
}

void BLE::Server::onConnect(const esp_ble_gatts_cb_param_t::gatts_connect_evt_param* param){
	memcpy(con.addr, param->remote_bda, 6);
	con.hndl = param->conn_id;

	// Client will initiate pairing, after which onPairDone() is called
	// Here, we only set up the connection parameters

	ConMan.connect(param->remote_bda);

	std::vector<ConnectCB> cbs;
	{
		std::lock_guard lock(cbMut);
		cbs.reserve(onConnectCBs.size());
		for(const auto& [_, cb] : onConnectCBs){
			if(cb) cbs.push_back(cb);
		}
	}
	for(const auto& cb : cbs){
		cb(con.addr);
	}
}

void BLE::Server::onDisconnect(const esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param* param){
	ESP_LOGI(TAG, "Disconnected. Reason: 0x%x", param->reason);

	// Snapshot the address before clearing so subscribers see the disconnecting peer.
	esp_bd_addr_t peer;
	memcpy(peer, param->remote_bda, 6);

	memset(con.addr, 0, 6);
	con.hndl = 0xffff;
	con.MTU_size = 23;

	// CCCD state is per-connection; drop it so a reconnecting peer starts clean.
	for(const auto& service : services){
		service->onDisconnect();
	}

	ConMan.disconnect();

	std::vector<DisconnectCB> cbs;
	{
		std::lock_guard lock(cbMut);
		cbs.reserve(onDisconnectCBs.size());
		for(const auto& [_, cb] : onDisconnectCBs){
			if(cb) cbs.push_back(cb);
		}
	}
	for(const auto& cb : cbs){
		cb(peer);
	}
}

void BLE::Server::passToChar(esp_gatts_cb_event_t event, const esp_ble_gatts_cb_param_t* param){
#define check(x) do { if(x == chars.end()){ ESP_LOGW(TAG, "Received event %d directed to non-registered characteristic", event); return; } } while(0)

	if(event == ESP_GATTS_READ_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");

		auto chr = chars.find(param->read.handle);
		check(chr);

		chr->second->onRead(&param->read);
	}else if(event == ESP_GATTS_WRITE_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");

		auto chr = chars.find(param->write.handle);
		if(chr == chars.end()){
			printf("0x%x%x\n", *param->write.value, *(param->write.value+1));
		}
		check(chr);

		chr->second->onWrite(&param->write);
	}else if(event == ESP_GATTS_EXEC_WRITE_EVT){
		ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");

		// No way to determine for which charachteristic this is => send to all - those without data pending will discard
		for(const auto& it : chars){
			if(it.second->props & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
				it.second->onExecWrite(&param->exec_write);
			}
		}
	}else{
		ESP_LOGW(TAG, "Unhandled characteristic event: 0x%x", event);
	}
}
