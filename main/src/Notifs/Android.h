#ifndef ARTEMIS_FIRMWARE_ANDROID_H
#define ARTEMIS_FIRMWARE_ANDROID_H

#include "Notifs/NotifSource.h"
#include "Notifs/MediaSource.h"
#include "Notifs/MediaInfo.h"
#include "BLE/Server.h"
#include "BLE/UART.h"
#include "Util/PSRAMAllocator.h"
#include <atomic>
#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <mutex>
#include <optional>

// Communication with Android devices via BLE UART
class Android : public NotifSource, public MediaSource, private Threaded {
public:
	static constexpr uint32_t ProtocolVersion = 2;
	static constexpr const char* FirmwareVersion = "v2.1";
	static constexpr size_t LogSize = 50;

	struct LogEntry {
		bool tx;
		uint32_t deltaMs;
		std::string line;
	};

	using BatteryCB = std::function<void(uint8_t percent)>;

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
	bool findPhoneActive();

	void setOnBattery(BatteryCB onBattery);

	void sendRaw(const std::string& line);
	void dropConnection();
	std::vector<LogEntry> getLog();
	uint32_t getLogSeq();
	std::optional<uint32_t> getLastCallId();
	std::optional<uint32_t> getLastNotifId();

private:
	void loop() override;

	BLE::Server* server;
	BLE::UART uart;

	BLE::Server::SubHandle disconnectSub = 0;

	// Written from the BLE/BTC task (onConnect/onDisconnect), read from the Android worker task in loop().
	std::atomic<bool> connected = false;

	uint32_t appProtocolVersion = 0;

	BatteryCB onBattery;

	struct StoredLogEntry {
		bool tx;
		uint32_t deltaMs;
		PSRAMString line;
	};

	std::mutex testMut;
	std::deque<StoredLogEntry, PSRAMAllocator<StoredLogEntry>> lineLog;
	std::atomic<uint32_t> logSeq = 0;
	uint64_t lastLogTime = 0;
	std::optional<uint32_t> lastCallId;
	std::optional<uint32_t> lastNotifId;

	void tx(const char* fmt, ...);
	void logLine(bool isTx, const std::string& line);

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
	void handleFindPhoneStopAck();
	void handleFindPhoneStopNack();
	void handleMediaState(const std::vector<std::string>& split_line);
	void handleMediaInfo(const std::vector<std::string>& split_line);
	void handleBattery(const std::vector<std::string>& split_line);

	static std::vector<std::string> splitProtocolMsg(const std::string& s, char delim = ';');

	void notifList();
	void callReject(uint32_t uid);
	void requestBattery();

	std::unordered_set<uint32_t> callIds;

	PSRAMByteBuffer rxBuf;

	bool findPhone = false;

	static Notif::Category mapNotifCategories(uint32_t category_val);
};

#endif //ARTEMIS_FIRMWARE_ANDROID_H