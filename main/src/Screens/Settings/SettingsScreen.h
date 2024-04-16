#ifndef CLOCKSTAR_FIRMWARE_SETTINGSSCREEN_H
#define CLOCKSTAR_FIRMWARE_SETTINGSSCREEN_H

#include "LV_Interface/LVScreen.h"
#include "Settings/Settings.h"
#include "Util/Events.h"
#include "UIElements/StatusBar.h"
#include "Services/BacklightBrightness.h"
#include "Services/ChirpSystem.h"
#include "Devices/IMU.h"
#include "LV_Interface/LVModal.h"
#include "TimePickerModal.h"

class SettingsScreen : public LVScreen {
public:
	SettingsScreen();

private:
	virtual void onStarting() override;
	virtual void onStart() override;
	virtual void onStop() override;
	virtual void loop() override;
	virtual void updateVisuals() override;

	void buildUI();

	Settings& settings;
	BacklightBrightness& backlight;
	ChirpSystem& audio;
	IMU& imu;
	Time& ts;

	lv_obj_t* bg = nullptr;
	lv_obj_t* container = nullptr;
	class PickerElement* themePicker = nullptr;
	PickerElement* dateFormatPicker = nullptr;
	class LabelElement* manualTime = nullptr;
	class BoolElement* audioSwitch = nullptr;
	class SliderElement* brightnessSlider = nullptr;
	BoolElement* ledSwitch = nullptr;
	StatusBar* statusBar = nullptr;
	class DiscreteSliderElement* sleepSlider = nullptr;
	LabelElement* saveAndExit = nullptr;
	BoolElement* motionSwitch = nullptr;
	BoolElement* rotationSwitch = nullptr;
	BoolElement* timeFormatSwitch = nullptr;
	LabelElement* loadingText;

	bool shouldTransition = false;

	TimePickerModal* timePickerModal = nullptr;

	static constexpr uint8_t TopPadding = 13;
	static constexpr const char* ThemeNames = "Default \nTheme 1\nTheme 2\nTheme 3\nTheme 4\nTheme 5\nTheme 6\nTheme 7\nTheme 8";

	EventQueue queue;
	Theme oldTheme = Theme::Default;
};


#endif //CLOCKSTAR_FIRMWARE_SETTINGSSCREEN_H
