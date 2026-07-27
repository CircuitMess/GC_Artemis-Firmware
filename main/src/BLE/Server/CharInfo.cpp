#include "../Server.h"
#include <cstring>

BLE::Server::CharInfo::CharInfo(Server* server, const Service* service, uint16_t hndl) : server(server), service(service), hndl(hndl){

}

void BLE::Server::CharInfo::addDescr(esp_bt_uuid_t uuid, esp_gatt_perm_t perm){
	server->queueDescr({ this->hndl, service->hndl, uuid, perm });
}

esp_err_t BLE::Server::CharInfo::sendResp(uint16_t conn_id, uint32_t trans, esp_gatt_status_t status, esp_gatt_rsp_t* resp){
	return esp_ble_gatts_send_response(server->iface.hndl, conn_id, trans, status, resp);
}

void BLE::Server::CharInfo::sendNotif(const uint8_t* data, size_t len){
	if(!server->con) return;

	const size_t chunkMax = (size_t) server->con.MTU_size - 4;
	size_t offset = 0;
	while(offset < len){
		const size_t chunk = std::min(len - offset, chunkMax);
		esp_ble_gatts_send_indicate(server->iface.hndl, server->con.hndl, hndl, chunk, const_cast<uint8_t*>(data) + offset, false);
		offset += chunk;
	}
}
