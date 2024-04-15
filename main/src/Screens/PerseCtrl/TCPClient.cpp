#include "TCPClient.h"
#include "Util/Events.h"
#include <lwip/sockets.h>
#include <esp_log.h>

static const char* TAG = "TCPClient";

TCPClient::TCPClient(){

}

bool TCPClient::isConnected() const{
	return sock != -1;
}

bool TCPClient::connect(){
	if(sock != -1){
		ESP_LOGE(TAG, "Connect, but already connected");
		return false;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sock == -1){
		ESP_LOGE(TAG, "Can't create socket, errno=%d: %s", errno, strerror(errno));
		return false;
	}
	timeval to{};
	to.tv_sec = 1;
	to.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &to, sizeof(to));


	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6000);
	inet_pton(AF_INET, "11.0.0.1", &addr.sin_addr);
	if(::connect(sock, (sockaddr*) &addr, sizeof(addr)) != 0){
		ESP_LOGE(TAG, "Can't connect to server, errno=%d: %s", errno, strerror(errno));
		return false;
	}

	Event event{ Event::Status::Connected };
	Events::post(Facility::TCP, event);

	ESP_LOGI(TAG, "Connection established");

	return true;
}

void TCPClient::disconnect(){
	if(sock == -1){
		ESP_LOGW(TAG, "Disconnect, but not connected");
		return;
	}

	Event event{ Event::Status::Disconnected };
	Events::post(Facility::TCP, event);

	close(sock);
	sock = -1;
}

bool TCPClient::read(uint8_t* buf, size_t count){
	if(sock == -1){
		ESP_LOGW(TAG, "Read, but not connected");
		return false;
	}

	if(count == 0 || buf == nullptr) return true;

	size_t total = 0;
	while(total < count){
		int now = ::read(sock, buf + total, count - total);

		if(now == 0){
			disconnect();
			return false;
		}else if(now < 0){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				vTaskDelay(1);
				continue;
			}else{
				disconnect();
				return false;
			}
		}

		total += now;
	}

	return true;
}

bool TCPClient::write(uint8_t* data, size_t count){
	if(sock == -1){
		ESP_LOGW(TAG, "Write, but not connected");
		return false;
	}

	if(count == 0 || data == nullptr) return true;

	size_t total = 0;
	while(total < count){
		int now = ::write(sock, data + total, count - total);

		if(now == 0){
			disconnect();
			return false;
		}else if(now < 0){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				vTaskDelay(1);
				continue;
			}else{
				disconnect();
				return false;
			}
		}

		total += now;
	}

	return true;
}
