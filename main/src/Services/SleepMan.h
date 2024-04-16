#ifndef CLOCKSTAR_FIRMWARE_SLEEPMAN_H
#define CLOCKSTAR_FIRMWARE_SLEEPMAN_H


#include "Sleep.h"
#include "Devices/IMU.h"
#include "Util/Events.h"
#include "LV_Interface/LVGL.h"
#include <memory>

class SleepMan {
public:
	explicit SleepMan(LVGL& lvgl);
	~SleepMan();
	void loop();

	void enAltLock(bool altLock);
	void enAutoSleep(bool autoSleep);

	static constexpr uint32_t ShutdownTime = 3000; //3s

	void wake(bool blockLockScreen = false);
	void imuSignal();
	void shutdown();

private:

	Sleep sleep;
	EventQueue events;

	LVGL& lvgl;
	IMU& imu;
	BacklightBrightness& bl;
	Settings& settings;

	void goSleep();

	bool inSleep = false;

	uint32_t actTime = 0;
	bool autoSleep = true;
	void checkAutoSleep();

	void checkEvents();
	void handleInput(const Input::Data& evt);
	void handleMotion(const IMU::Event& evt);

	static constexpr uint32_t WakeCooldown = 100;
	uint32_t wakeTime = 0;

	bool altLock = false;
	static constexpr uint32_t AltHoldTime = 300;
	uint32_t altPress;

	bool nsBlocked = false;

	//if sleep is caused by btn press, register for Lowered event instead of Lifted, then clear this flag when Lowered occurs
	bool waitForLower = false;

	//if wake is caused by btn press, and we are in lowered position, register for Lifted event instead of Lowered, then clear this flag when Lifted occurs
	bool waitForLift = false;

	bool checkIMUTilt(IMU::TiltDirection direction) const;
};


#endif //CLOCKSTAR_FIRMWARE_SLEEPMAN_H
