#ifndef ARTEMIS_FIRMWARE_TESTERSCREEN_H
#define ARTEMIS_FIRMWARE_TESTERSCREEN_H

#include "LV_Interface/LVScreen.h"
#include "LV_Interface/LVStyle.h"
#include "Notifs/Phone.h"
#include "Util/Events.h"
#include <string>
#include <vector>
#include <optional>

class TesterScreen : public LVScreen {
public:
	TesterScreen();
	~TesterScreen() override;

private:
	static constexpr const char* DropCommand = "[drop BLE link]";
	static constexpr const char* Commands[] = {
			"version", "battery", "notifList", "notifPos", "notifNeg", "callReject", "callAnswer",
			"mediaPlay", "mediaPause", "mediaNext", "mediaPrev", "findPhoneStart", "findPhoneStop", "volumeUp", "volumeDown", DropCommand
	};
	static constexpr size_t CommandCount = sizeof(Commands) / sizeof(Commands[0]);
	static constexpr const char* InvalidId = "-1";
	static constexpr size_t LogLines = 5;
	static constexpr uint32_t PollInterval = 250;
	static constexpr uint32_t ScrollSpeed = 21;
	static constexpr uint32_t ScrollPause = 1000;

	enum class Preset : uint8_t { Latest, Invalid, Zero, Max, Random, COUNT };
	enum class ParamKind : uint8_t { None, NotifId, CallId };

	Phone& phone;
	EventQueue queue;

	LVStyle textStyle;

	lv_obj_t* content = nullptr;
	lv_obj_t* blockedLabel = nullptr;
	lv_obj_t* statusLabel = nullptr;
	lv_obj_t* logHeader = nullptr;
	lv_obj_t* logLabels[LogLines] = { nullptr };
	lv_obj_t* cmdLabel = nullptr;
	lv_obj_t* lineLabel = nullptr;
	lv_obj_t* paramLabel = nullptr;
	lv_obj_t* hintTop = nullptr;
	lv_obj_t* hintBottom = nullptr;

	size_t index = 0;
	size_t logOffset = 0;
	Preset presets[CommandCount] = {};
	uint32_t randomValue = 0;
	std::vector<Android::LogEntry> logCache;
	std::optional<uint32_t> logSeqCache;
	std::string statusCache;
	bool altHeld = false;
	bool altModified = false;
	bool paramMode = false;
	bool blocked = false;
	uint64_t lastPoll = 0;

	void loop() override;
	void onStart() override;
	void onStop() override;

	void buildUI();
	void applyMode();
	static void applyScrollPause(lv_obj_t* label);
	static std::string oneLine(const std::string& text);
	void updateSelector();
	void updateStatus();
	void updateLog();
	void renderLog();

	void prev();
	void next();
	void scrollLog(bool older);
	void cyclePreset(bool forward);
	void regenerateRandom();
	void send();

	ParamKind paramKind(size_t cmdIndex);
	std::string paramValue(size_t cmdIndex);
	std::string paramDesc(size_t cmdIndex);
	std::string buildLine(size_t cmdIndex);
};

#endif //ARTEMIS_FIRMWARE_TESTERSCREEN_H