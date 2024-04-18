#include "SleepMan.h"
#include "Screens/Lock/LockScreen.h"
#include "Util/Services.h"
#include "Screens/ShutdownScreen.h"
#include <esp_sleep.h>
#include "Screens/MainMenu/MainMenu.h"

static const char* tag = "SleepMan";

SleepMan::SleepMan(LVGL& lvgl) : events(12), lvgl(lvgl),
								 imu(*((IMU*) Services.get(Service::IMU))),
								 bl(*((BacklightBrightness*) Services.get(Service::Backlight))),
								 settings(*((Settings*) Services.get(Service::Settings))){
	Events::listen(Facility::Input, &events);
	Events::listen(Facility::Motion, &events);
	Events::listen(Facility::Battery, &events);

	imu.enableTiltDetection(settings.get().motionDetection);
	imu.setTiltDirection(IMU::TiltDirection::Lowered);

	actTime = millis();
}

SleepMan::~SleepMan(){
	Events::unlisten(&events);
}

void SleepMan::goSleep(){
	auto battery = (Battery*) Services.get(Service::Battery);
	if(!battery || battery->isShutdown()) return;

	MainMenu::resetMenuIndex();

	lvgl.stopScreen();

	if(settings.get().motionDetection){
		imu.init();

		if(waitForLower){
			imu.setTiltDirection(IMU::TiltDirection::Lowered);
		}else{
			imu.setTiltDirection(IMU::TiltDirection::Lifted);
		}
	}

	inSleep = true;

	bool buttonWakeWhileLowered = false;
	sleep.sleep([this, &buttonWakeWhileLowered](){

		if(checkIMUTilt(IMU::TiltDirection::Lowered)){
			buttonWakeWhileLowered = true;
			waitForLower = false;
			ESP_LOGI(tag, "button wake while lowered!");

		}else{
			buttonWakeWhileLowered = false;
		}

		if(!nsBlocked){
			lvgl.startScreen([](){ return std::make_unique<LockScreen>(); });
		}
		lv_timer_handler();
	});
	nsBlocked = inSleep = false;

	if(settings.get().motionDetection){
		imu.init();

		if(buttonWakeWhileLowered){
			imu.setTiltDirection(IMU::TiltDirection::Lifted);
			waitForLift = true;
		}else{
			imu.setTiltDirection(IMU::TiltDirection::Lowered);
			waitForLift = false;
		}
		ESP_LOGD(tag, "wakeup! waitForLower: %d, waitForLift: %d", waitForLower, waitForLift);
	}

	wakeTime = actTime = millis();
	events.reset();
	waitForLower = false;
}

void SleepMan::wake(bool blockLock){
	if(!inSleep) return;
	nsBlocked = blockLock;
	xSemaphoreGive(sleep.wakeSem);
}

void SleepMan::shutdown(){
	bl.fadeOut();
	imu.shutdown();

	gpio_set_level((gpio_num_t) PIN_PWDN, 0);
	const gpio_config_t cfg = {
			.pin_bit_mask = 1ULL << PIN_PWDN,
			.mode = GPIO_MODE_OUTPUT
	};
	gpio_config(&cfg);
}

void SleepMan::loop(){
	checkAutoSleep();
	checkEvents();
}

void SleepMan::checkEvents(){
	Event evt;
	while(events.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto param = (Input::Data*) evt.data;
			handleInput(*param);
		}else if(evt.facility == Facility::Motion){
			auto param = (IMU::Event*) evt.data;
			handleMotion(*param);
		}

		free(evt.data);
	}
}

void SleepMan::checkAutoSleep(){
	if(!autoSleep) return;

	auto sti = settings.get().sleepTime;
	if(sti >= Settings::SleepSteps) return;

	auto sleepSeconds = Settings::SleepSeconds[settings.get().sleepTime];
	if(sleepSeconds == 0) return;

	if((millis() - actTime) / 1000 < sleepSeconds) return;

	if(checkIMUTilt(IMU::TiltDirection::Lifted)){
		waitForLower = true;
		waitForLift = false;
		ESP_LOGI(tag, "auto sleep while lifted!\n");
		ESP_LOGD(tag, "sleep! waitForLower: %d, waitForLift: %d", waitForLower, waitForLift);

	}
	goSleep();
}

void SleepMan::handleInput(const Input::Data& evt){
	actTime = millis();

	if(evt.btn != Input::Alt || !altLock) return;

	if(evt.action == Input::Data::Press){
		if(millis() - wakeTime < WakeCooldown) return;
		altPress = millis();
	}else if(millis() - altPress < AltHoldTime){
		if(checkIMUTilt(IMU::TiltDirection::Lifted)){
			waitForLower = true;
			waitForLift = false;
			ESP_LOGI(tag, "button sleep while lifted!\n");
			ESP_LOGD(tag, "sleep! waitForLower: %d, waitForLift: %d", waitForLower, waitForLift);
		}
		goSleep();
	}
}

void SleepMan::handleMotion(const IMU::Event& evt){
	if(evt.action == IMU::Event::WristTilt && evt.wristTiltDir == IMU::TiltDirection::Lifted && waitForLift){
		waitForLift = false;
		imu.setTiltDirection(IMU::TiltDirection::Lowered);
	}

	if(!autoSleep) return;

	const bool wristSleep = settings.get().motionDetection;

	if((evt.action == IMU::Event::WristTilt && evt.wristTiltDir == IMU::TiltDirection::Lowered && wristSleep) || evt.action == IMU::Event::DoubleTap){
		goSleep();
	}
}

void SleepMan::enAltLock(bool altLock){
	SleepMan::altLock = altLock;
}

void SleepMan::enAutoSleep(bool autoSleep){
	SleepMan::autoSleep = autoSleep;
}

void SleepMan::imuSignal(){
	if(!waitForLower){
		wake();
		return;
	}

	imu.setTiltDirection(IMU::TiltDirection::Lifted);
	waitForLower = false;
}

bool SleepMan::checkIMUTilt(IMU::TiltDirection direction) const{
	static constexpr float LiftThreshold = 8 * 0.0015625;
	const auto rotation = settings.get().screenRotate;
	const auto sample = imu.getSample();

	if((direction == IMU::TiltDirection::Lowered) ^ rotation){
		return sample.accelY >= LiftThreshold;
	}else{
		return sample.accelY <= -LiftThreshold;
	}
}
