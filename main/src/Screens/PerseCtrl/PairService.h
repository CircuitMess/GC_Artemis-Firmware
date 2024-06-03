#ifndef PERSE_MISSIONCTRL_PAIRSERVICE_H
#define PERSE_MISSIONCTRL_PAIRSERVICE_H


#include "WiFiSTA.h"
#include "TCPClient.h"
#include "Util/Threaded.h"
#include "Util/Events.h"

class PairService {
public:
	PairService(WiFiSTA& wifi, TCPClient& tcp);
	virtual ~PairService();

	enum class State {
		Pairing, Fail, Success
	};
	State getState() const;

	struct Event {
		bool success;
	};

private:
	inline static constexpr const uint8_t ConnectionAttempts = 10;
	inline static constexpr const uint32_t AbortTimeout = 2000;

	WiFiSTA& wifi;
	TCPClient& tcp;

	State state = State::Pairing;
	ThreadedClosure thread;
	EventQueue queue;

	uint8_t attempted = 1;
	int64_t connectStart = -1;

	void processEvent(const WiFiSTA::Event& event);

	void loop();
};


#endif //PERSE_MISSIONCTRL_PAIRSERVICE_H
