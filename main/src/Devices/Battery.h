#ifndef BIT_LIBRARY_BATTERY_H
#define BIT_LIBRARY_BATTERY_H

#include <hal/gpio_types.h>
#include <atomic>
#include "Util/Threaded.h"
#include "Periph/ADC.h"
#include "Util/Hysteresis.h"
#include "Periph/Timer.h"
#include "Services/ADCReader.h"
#include "Periph/PinOut.h"
#include <mutex>
#include <esp_efuse.h>
#include <memory>

class Battery : private SleepyThreaded {
public:
	Battery(ADC& adc);

	enum Level { Critical = 0, VeryLow, Low, Mid, High, VeryHigh, Full, COUNT };

	struct Event {
		enum { LevelChange } action;
		union { Level level; };
	};

	void begin();

	bool isShutdown() const;

	uint8_t getPerc() const;
	Level getLevel() const;

private:
	static constexpr uint32_t MeasureIntverval = 100;

	static constexpr float VoltEmpty = 3600;
	static constexpr float VoltFull = 4200;
	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 0;
	static constexpr float EmaA = 0.05f;

	static constexpr int CalReads = 10;
	static constexpr float CalExpected = 2500;

	ADC& adc;
	PinOut refSwitch;
	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

	std::unique_ptr<ADCReader> readerRef;
	adc_cali_handle_t caliRef;

	void calibrate();

	void sample(bool fresh = false);
	bool shutdown = false;

	void sleepyLoop() override;

};

#endif //BIT_LIBRARY_BATTERY_H
