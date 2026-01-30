#ifndef ARTEMIS_FIRMWARE_ANDROID_H
#define ARTEMIS_FIRMWARE_ANDROID_H

#include "Notifs/NotifSource.h"
#include "BLE/Server.h"
#include "BLE/UART.h"
#include <string>
#include <map>
#include <vector>

// Communication with Android devices via BLE UART
class Android : public NotifSource, private Threaded {
public:
	Android(BLE::Server* server);
	virtual ~Android();

	void actionPos(uint32_t uid) override;
	void actionNeg(uint32_t uid) override;

	void findPhoneStart();
	void findPhoneStop();

private:
	void loop() override;

	BLE::Server* server;
	BLE::UART uart;

	bool connected = false;

	void onConnect();
	void onDisconnect();

	void handleCommand(const std::string& line);

	// command handlers
	void handle_notify(const std::string& line);
	void handle_notifyDel(uint32_t id);
	void handle_call(const std::string& line);

	static std::string getProperty(const std::string& line, std::string prop);

	enum class CallState : uint8_t {
		None, Incoming, Outgoing, IncomingAccepted, IncomingMissed
	};
	enum class CallCmd : uint8_t {
		Outgoing, End, Incoming, Start, Invalid, Any
	};

	struct CallInfo {
		const char* message;
		Notif::Category category;
	};

	static const std::map<std::pair<CallState, CallCmd>, CallState> CallTransitions;
	static const std::unordered_map<CallState, CallInfo> CallInfoMap;

	uint32_t currentCallId = -1;
	CallState currentCallState = CallState::None;

	std::unordered_set<uint32_t> missedCalls;

	float timezone_offset = 0;
	uint64_t timestamp = 0;

	void setTime();
	std::vector<std::string> splitProtocolMsg(const std::string& s, char delim = ';');
};

#endif //ARTEMIS_FIRMWARE_ANDROID_H