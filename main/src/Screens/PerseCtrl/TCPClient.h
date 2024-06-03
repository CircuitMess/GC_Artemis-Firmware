#ifndef PERSE_MISSIONCTRL_TCPCLIENT_H
#define PERSE_MISSIONCTRL_TCPCLIENT_H

#include <cstdint>
#include <cstddef>

class TCPClient {
public:

	struct Event {
		enum class Status {
			Connected, Disconnected
		} status;
	};

	TCPClient();

	bool isConnected() const;

	bool connect();
	void disconnect();

	bool read(uint8_t* buf, size_t count);
	bool write(uint8_t* data, size_t count);

private:
	int sock = -1;

};


#endif //PERSE_MISSIONCTRL_TCPCLIENT_H
