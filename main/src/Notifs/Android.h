#ifndef ARTEMIS_FIRMWARE_ANDROID_H
#define ARTEMIS_FIRMWARE_ANDROID_H

#include "Notifs/NotifSource.h"
#include "Notifs/MediaSource.h"
#include "Notifs/Media.h"
#include "BLE/Server.h"
#include "BLE/UART.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

// Communication with Android devices via BLE UART
class Android : public NotifSource, public MediaSource, private Threaded {
public:
	static constexpr const char* ProtocolVersion = "1";
	static constexpr const char* FirmwareVersion = "v2.1";

	Android(BLE::Server* server);
	virtual ~Android();

	void actionPos(uint32_t uid) override;
	void actionNeg(uint32_t uid) override;

	void mediaPlay() override;
	void mediaPause() override;
	void mediaNext() override;
	void mediaPrev() override;

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
	void handleHello(const std::vector<std::string>& split_line);
	void handleNotifAdd(const std::vector<std::string>& split_line);
	void handleNotifDel(const std::vector<std::string>& split_line);
	void handleNotifModify(const std::vector<std::string>& split_line);
	void handleCallIncoming(const std::vector<std::string>& split_line);
	void handleCallIncomingStop(const std::vector<std::string>& split_line);
	void handleTime(const std::vector<std::string>& split_line);
	void handleFindPhoneStopAck(const std::vector<std::string>& split_line);
	void handleMediaState(const std::vector<std::string>& split_line);
	void handleMediaInfo(const std::vector<std::string>& split_line);

	bool currentRingingState = false;
	Media currentMedia;
	uint32_t mediaIdCounter = 0;

	void requestTime();
	static std::vector<std::string> splitProtocolMsg(const std::string& s, char delim = ';');

	void notifList();
	void callReject(uint32_t uid);

	static Notif::Category mapNotifCategories(uint32_t category_val);
};

#endif //ARTEMIS_FIRMWARE_ANDROID_H