#include "SettingsScreen.h"
#include "Devices/Input.h"
#include "Util/Services.h"
#include "Screens/MainMenu/MainMenu.h"
#include "BoolElement.h"
#include "SliderElement.h"
#include "LabelElement.h"
#include "DiscreteSliderElement.h"
#include "Services/StatusCenter.h"
#include "LV_Interface/LVGL.h"
#include "TimePickerModal.h"
#include "PickerElement.h"
#include "Services/Time.h"
#include "Filepaths.hpp"
#include "LV_Interface/InputLVGL.h"
#include "LV_Interface/FSLVGL.h"

SettingsScreen::SettingsScreen() : settings(*(Settings*) Services.get(Service::Settings)), backlight(*(BacklightBrightness*) Services.get(Service::Backlight)),
								   audio(*(ChirpSystem*) Services.get(Service::Audio)), imu(*(IMU*) Services.get(Service::IMU)),
								   ts(*(Time*) Services.get(Service::Time)), queue(4){
	buildUI();
}

void SettingsScreen::loop(){
	Event evt{};
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				shouldTransition = true;
			}
		}

		free(evt.data);
	}

	statusBar->loop();

	if(shouldTransition){
		delete timePickerModal;
		timePickerModal = nullptr;

		if(oldTheme != settings.get().themeData.theme){
			lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(*loadingText, LV_OBJ_FLAG_HIDDEN);
			lv_obj_invalidate(*this);
			vTaskDelay(LV_DISP_DEF_REFR_PERIOD);
			lv_timer_handler();

			FSLVGL::unloadCache();
			FSLVGL::loadCache(settings.get().themeData.theme);
		}

		transition([](){ return std::make_unique<MainMenu>(); });
		return;
	}
}

void SettingsScreen::onStop(){
	auto savedSettings = settings.get();
	savedSettings.notificationSounds = audioSwitch->getValue();
	savedSettings.screenBrightness = brightnessSlider->getValue();
	savedSettings.sleepTime = sleepSlider->getValue();
	savedSettings.ledEnable = ledSwitch->getValue();
	savedSettings.motionDetection = motionSwitch->getValue();
	savedSettings.screenRotate = rotationSwitch->getValue();
	savedSettings.timeFormat24h = timeFormatSwitch->getValue();
	settings.set(savedSettings);
	settings.store();

	backlight.setBrightness(brightnessSlider->getValue());
	imu.enableTiltDetection(motionSwitch->getValue());
	lvgl->rotateScreen(rotationSwitch->getValue());
	InputLVGL::getInstance()->invertDirections(rotationSwitch->getValue());
	//TODO - apply sleep time

	Events::unlisten(&queue);

	auto status = (StatusCenter*) Services.get(Service::Status);
	status->blockAudio(false);
	status->updateLED();
}

void SettingsScreen::onStarting(){
	auto sets = settings.get();
	brightnessSlider->setValue(sets.screenBrightness);
	audioSwitch->setValue(sets.notificationSounds);
	ledSwitch->setValue(sets.ledEnable);
	sleepSlider->setValue(sets.sleepTime);
	motionSwitch->setValue(sets.motionDetection);
	rotationSwitch->setValue(sets.screenRotate);
}

void SettingsScreen::onStart(){
	Events::listen(Facility::Input, &queue);

	auto status = (StatusCenter*) Services.get(Service::Status);
	status->blockAudio(true);

	oldTheme = settings.get().themeData.theme;
}

void SettingsScreen::updateVisuals(){
	const Theme theme = settings.get().themeData.theme;

	if(bg != nullptr){
		lv_obj_set_style_bg_img_src(bg, THEMED_FILE(Menu, SettingsBackground, theme), 0);
	}

	if(container != nullptr){
		lv_obj_set_size(container, 128, 128 - TopPadding - (theme == Theme::Theme9 ? 7 : 0));
		lv_obj_set_pos(container, 0, TopPadding + (theme == Theme::Theme9 ? 7 : 0));
	}

	if(statusBar != nullptr){
		statusBar->updateVisuals();
	}

	if(timeFormatSwitch != nullptr){
		timeFormatSwitch->updateVisuals();
	}

	if(themePicker != nullptr){
		themePicker->updateVisuals();
	}

	if(manualTime != nullptr){
		manualTime->updateVisuals();
	}

	if(audioSwitch != nullptr){
		audioSwitch->updateVisuals();
	}

	if(brightnessSlider != nullptr){
		brightnessSlider->updateVisuals();
	}

	if(ledSwitch != nullptr){
		ledSwitch->updateVisuals();
	}

	if(sleepSlider != nullptr){
		sleepSlider->updateVisuals();
	}

	if(motionSwitch != nullptr){
		motionSwitch->updateVisuals();
	}

	if(rotationSwitch != nullptr){
		rotationSwitch->updateVisuals();
	}

	if(saveAndExit != nullptr){
		saveAndExit->updateVisuals();
	}

	if(dateFormatPicker != nullptr){
		dateFormatPicker->updateVisuals();
	}

	if(loadingText != nullptr){
		loadingText->updateVisuals();
	}
}

void SettingsScreen::buildUI(){
	const Theme theme = settings.get().themeData.theme;

	lv_obj_set_size(*this, 128, 128);

	bg = lv_obj_create(*this);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	lv_obj_set_style_bg_img_src(bg, THEMED_FILE(Menu, SettingsBackground, theme), 0);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);

	container = lv_obj_create(*this);
	lv_obj_set_size(container, 128, 128 - TopPadding - (theme == Theme::Theme9 ? 7 : 0));
	lv_obj_set_pos(container, 0, TopPadding + (theme == Theme::Theme9 ? 7 : 0));
	lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(container, 5, 0);
	lv_obj_set_style_pad_hor(container, 1, 0);
	lv_obj_set_style_pad_bottom(container, 1, 0);

	statusBar = new StatusBar(*this);
	lv_obj_add_flag(*statusBar, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_pos(*statusBar, 0, 0);

	auto startingSettings = settings.get();

	timeFormatSwitch = new BoolElement(container, "24-hour format", [this](bool value){
		auto s = settings.get();
		s.timeFormat24h = value;
		settings.set(s);
		statusBar->set24hFormat(value);
	}, startingSettings.timeFormat24h);
	lv_group_add_obj(inputGroup, *timeFormatSwitch);

	themePicker = new PickerElement(container, "Change theme", (uint16_t) startingSettings.themeData.theme,
									"Default\nArcade\nAnalog\nDuotone\nMachine\nTXT\nCY-PUNK\nCipher\n1-Up",
									[this](uint16_t selected){
										SettingsStruct sett = settings.get();
										sett.themeData = createTheme((Theme) selected);
										settings.set(sett);

										updateVisuals();
									});
	lv_group_add_obj(inputGroup, *themePicker);

	dateFormatPicker = new PickerElement(container, "Date format", (uint16_t) startingSettings.dateFormat,
									"D.M.Y\nM.D.Y",
									[this](uint16_t selected){
										SettingsStruct sett = settings.get();
										sett.dateFormat = (DateFormat) selected;
										settings.set(sett);
									});
	lv_group_add_obj(inputGroup, *dateFormatPicker);

	manualTime = new LabelElement(container, "Adjust time/date", [this](){
		timePickerModal = new TimePickerModal(this, ts.getTime(), [this]() {
			delete timePickerModal;
			timePickerModal = nullptr;
		});
	}, false,  LV_ALIGN_LEFT_MID);
	lv_group_add_obj(inputGroup, *manualTime);

	audioSwitch = new BoolElement(container, "Sound", [](bool value){
		if(value){
			auto status = (StatusCenter*) Services.get(Service::Status);
			status->beep();
		}
	}, startingSettings.notificationSounds);
	lv_group_add_obj(inputGroup, *audioSwitch);

	brightnessSlider = new SliderElement(container, "Brightness", [this](uint8_t value){
		backlight.setBrightness(value);
	}, startingSettings.screenBrightness);
	lv_group_add_obj(inputGroup, *brightnessSlider);

	ledSwitch = new BoolElement(container, "LED enable", [this](bool value){
		auto s = settings.get();
		s.ledEnable = value;
		settings.set(s);

		auto status = (StatusCenter*) Services.get(Service::Status);
		status->updateLED();

		if(value){
			status->blink();
		}
	}, startingSettings.notificationSounds);
	lv_group_add_obj(inputGroup, *ledSwitch);

	sleepSlider = new DiscreteSliderElement(container, "Sleep time", [this](uint8_t value){
		if(value >= Settings::SleepSteps) return;
		auto s = settings.get();
		s.sleepTime = value;
		settings.set(s);
	}, std::vector<const char*>(Settings::SleepText, Settings::SleepText + Settings::SleepSteps), startingSettings.sleepTime);
	lv_group_add_obj(inputGroup, *sleepSlider);

	motionSwitch = new BoolElement(container, "Tilt to wake", [this](bool value){
		auto s = settings.get();
		s.motionDetection = value;
		settings.set(s);
	}, startingSettings.notificationSounds);
	lv_group_add_obj(inputGroup, *motionSwitch);

	rotationSwitch = new BoolElement(container, "Left-hand mode", [this](bool value){
		auto s = settings.get();
		s.motionDetection = value;
		settings.set(s);
		lvgl->rotateScreen(value);
		lv_obj_invalidate(*this);
		InputLVGL::getInstance()->invertDirections(value);
	}, startingSettings.screenRotate);
	lv_group_add_obj(inputGroup, *rotationSwitch);

	saveAndExit = new LabelElement(container, "Save and Exit", [this](){
		shouldTransition = true;
	}, false, LV_ALIGN_LEFT_MID);
	lv_group_add_obj(inputGroup, *saveAndExit);

	for(int i = 0; i < lv_obj_get_child_cnt(container); ++i){
		lv_obj_add_flag(lv_obj_get_child(container, i), LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	}

	loadingText = new LabelElement(*this, "Loading...", [](){}, false, LV_ALIGN_CENTER);
	lv_obj_set_align(*loadingText, LV_ALIGN_CENTER);
	lv_obj_add_flag(*loadingText, LV_OBJ_FLAG_HIDDEN);
}