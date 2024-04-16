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
#include "Filepaths.hpp"
#include "Settings/Settings.h"

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
	ItemInfo ItemInfos[7] = {
			{ "Find my phone",    "Ringing phone...",                     File::Menu::Theme1::Find,       File::Menu::Theme1::Find },
			{ "Lunar lander",	  nullptr,                  			  File::Menu::Theme1::Lunar, 	  nullptr },
			{ "Level",            nullptr,                                File::Menu::Theme1::Level,      nullptr },
			{ "Theremin",         nullptr,                                File::Menu::Theme1::Theremin,   nullptr },
			{ "Phone connection", ConnDesc[(int) Phone::PhoneType::None], File::Menu::Theme1::Connection, File::Menu::Theme1::Connection },
			{ "Rover control",	  nullptr,                                File::Menu::Theme1::Rover,   	  nullptr },
			{ "Settings",         nullptr,                                File::Menu::Theme1::Settings,   nullptr }
	};
	static constexpr uint8_t ItemCount = sizeof(ItemInfos) / sizeof(ItemInfos[0]);
	void setupItemPaths(Theme theme);

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
