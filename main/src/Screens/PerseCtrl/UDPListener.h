#ifndef PERSE_MISSIONCTRL_UDPLISTENER_H
#define PERSE_MISSIONCTRL_UDPLISTENER_H

#include <cstdint>
#include <cstddef>

class UDPListener {
public:
	UDPListener();
	virtual ~UDPListener();

	int read(uint8_t* buf, size_t count);

private:
	int sock = -1;

};


#endif //PERSE_MISSIONCTRL_UDPLISTENER_H
