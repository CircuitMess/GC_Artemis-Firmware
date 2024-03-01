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
#include "Services/Time.h"
#include "LV_Interface/InputLVGL.h"

SettingsScreen::SettingsScreen() : settings(*(Settings*) Services.get(Service::Settings)), backlight(*(BacklightBrightness*) Services.get(Service::Backlight)),
								   audio(*(ChirpSystem*) Services.get(Service::Audio)), imu(*(IMU*) Services.get(Service::IMU)),
								   ts(*(Time*)Services.get(Service::Time)), queue(4){
	lv_obj_set_size(*this, 128, 128);

	bg = lv_obj_create(*this);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(bg, "S:/bg.bin", 0);

	container = lv_obj_create(*this);
	lv_obj_set_size(container, 128, 128 - TopPadding);
	lv_obj_set_pos(container, 0, TopPadding);
	lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(container, 5, 0);

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

	manualTime = new LabelElement(container, "Adjust date/time", [this](){
		timePickerModal = new TimePickerModal(this, ts.getTime());
	});
	lv_group_add_obj(inputGroup, *manualTime);

	audioSwitch = new BoolElement(container, "Sound", [this](bool value){
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

	rotationSwitch = new BoolElement(container, "Rotate screen", [this](bool value){
		auto s = settings.get();
		s.motionDetection = value;
		settings.set(s);
	}, startingSettings.screenRotate);
	lv_group_add_obj(inputGroup, *rotationSwitch);

	saveAndExit = new LabelElement(container, "Save and Exit", [this](){
		transition([](){ return std::make_unique<MainMenu>(); });
	});
	lv_group_add_obj(inputGroup, *saveAndExit);

	for(int i = 0; i < lv_obj_get_child_cnt(container); ++i){
		lv_obj_add_flag(lv_obj_get_child(container, i), LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	}
}

void SettingsScreen::loop(){
	Event evt{};
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				if(timePickerModal){
					delete timePickerModal;
					timePickerModal = nullptr;
				}else{
					free(evt.data);
					transition([](){ return std::make_unique<MainMenu>(); });
					return;
				}
			}
		}
		free(evt.data);
	}

	statusBar->loop();
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
}
