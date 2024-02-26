#ifndef CLOCKSTAR_FIRMWARE_MAINMENU_H
#define CLOCKSTAR_FIRMWARE_MAINMENU_H

#include <map>
#include <memory>
#include "LV_Interface/LVScreen.h"
#include "UIElements/StatusBar.h"
#include "MenuItem.h"
#include "Notifs/Phone.h"
#include "Devices/Input.h"
#include "MenuItemAlt.h"

class MainMenu : public LVScreen {
public:
	MainMenu();
	virtual ~MainMenu();

	static void resetMenuIndex();

private:
	static constexpr const char* ConnDesc[3] = {
			"Not connected.\nUse phone to pair",
			"Connected to\nAndroid device",
			"Connected to\niOS device"
	};

	struct ItemInfo {
		const char* labelText = nullptr;
		const char* labelTextAlt = nullptr;
		const char* iconPath = nullptr;
		const char* iconAltPath = nullptr;
	};
	static constexpr ItemInfo ItemInfos[] = {
			{ "Find my phone",    "Ringing phone...",                     "S:/menu/find.bin",       "S:/menu/find.bin" },
			{ "Level",            nullptr,                                "S:/menu/level.bin",      nullptr },
			{ "Theremin",         nullptr,                                "S:/menu/theremin.bin",   nullptr },
			{ "Phone connection", ConnDesc[(int) Phone::PhoneType::None], "S:/menu/connection.bin", "S:/menu/connection.bin" },
			{ "Settings",         nullptr,                                "S:/menu/settings.bin",   nullptr }
	};
	static constexpr uint8_t ItemCount = sizeof(ItemInfos) / sizeof(ItemInfos[0]);

	void setConnAlts();

	bool findPhoneRinging = false;
	void startPhoneRing();
	void stopPhoneRing();
	void handleRing();

	bool findPhoneState = false;
	uint32_t findPhoneCounter = 0;
	static constexpr uint32_t FindPhonePeriod = 1000; //1 second

	static uint8_t lastIndex;

	Phone& phone;

	StatusBar* statusBar;

	lv_obj_t* bg;
	lv_obj_t* container;
	MenuItem* items[ItemCount];
	// MenuItemAlt* phoneConnection;

	void onStart() override;
	void onStarting() override;
	void onStop() override;
	void loop() override;
	EventQueue queue;

	void onClick();

	void handlePhoneChange(Phone::Event& event);
	void handleInput(Input::Data& event);
};


#endif //CLOCKSTAR_FIRMWARE_MAINMENU_H
