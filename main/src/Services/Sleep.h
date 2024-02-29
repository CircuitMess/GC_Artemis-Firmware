#ifndef CLOCKSTAR_FIRMWARE_SLEEP_H
#define CLOCKSTAR_FIRMWARE_SLEEP_H

#include "Devices/Input.h"
#include "Devices/Battery.h"
#include "Services/Time.h"
#include "BacklightBrightness.h"
#include "Pins.hpp"

class SleepMan;

class Sleep {
public:
	Sleep();

	void sleep(std::function<void()> preWake = {});

	struct Event {
		enum { SleepOn, SleepOff } action;
	};

private:
friend SleepMan;
	DRAM_ATTR static constexpr gpio_num_t WakePin = (gpio_num_t) BTN_ALT;

	SemaphoreHandle_t wakeSem;
	static void intr(void* arg);

	void sleepStart();

	void confPM(bool sleep, bool firstTime = false);

};


#endif //CLOCKSTAR_FIRMWARE_SLEEP_H
