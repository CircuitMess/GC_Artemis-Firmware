#include "UDPListener.h"
#include <lwip/sockets.h>
#include <esp_log.h>

static const char* TAG = "UDPListener";

UDPListener::UDPListener(){
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(sock == -1){
		ESP_LOGE(TAG, "Can't create socket, errno=%d: %s", errno, strerror(errno));
		return;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6001);
	inet_pton(AF_INET, "11.0.0.2", &addr.sin_addr);
	if(bind(sock, (sockaddr*) &addr, sizeof(addr)) != 0){
		ESP_LOGE(TAG, "Can't bind address to socket, errno=%d: %s", errno, strerror(errno));
		sock = -1;
		return;
	}

	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

UDPListener::~UDPListener(){
	close(sock);
}

int UDPListener::read(uint8_t* buf, size_t count){
	if(sock == -1){
		ESP_LOGW(TAG, "Read, but socket not set-up");
		return false;
	}

	if(count == 0 || buf == nullptr) return 0;

	int bytes = ::recv(sock, buf, count, 0);

	if(bytes < 0){
		if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;

		ESP_LOGE(TAG, "Read error, errno=%d: %s", errno, strerror(errno));
		return -1;
	}

	return bytes;
}
