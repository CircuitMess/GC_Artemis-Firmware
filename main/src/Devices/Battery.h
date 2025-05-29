#ifndef CLOCKSTAR_LIBRARY_BATTERY_H
#define CLOCKSTAR_LIBRARY_BATTERY_H

#include <hal/gpio_types.h>
#include <atomic>
#include "Util/Threaded.h"
#include "Periph/ADC.h"
#include "Util/Hysteresis.h"
#include "Periph/Timer.h"
#include "Services/ADCReader.h"
#include "Periph/PinOut.h"
#include "Util/TimeHysteresis.h"
#include "Services/ADCReader.h"
#include <mutex>
#include <esp_efuse.h>
#include <memory>

class Battery : private Threaded {
public:
	Battery(ADC& adc);
	virtual ~Battery() override;
	void begin();

	enum Level { Critical = 0, VeryLow, Low, Mid, Full, COUNT };
	enum class ChargingState : uint8_t { Unplugged, Charging, Full };
	void setSleep(bool sleep);

	uint8_t getPerc() const;
	Level getLevel() const;
	ChargingState getChargingState() const;

	struct Event {
		enum {
			Charging, LevelChange
		} action;
		union {
			ChargingState chargeStatus;
			Level level;
		};
	};

	bool isShutdown() const;

private:
	static constexpr uint32_t ShortMeasureIntverval = 100;
	static constexpr uint32_t LongMeasureIntverval = 6000;

	static constexpr float VoltFull = 4200.0f; //[mV]
	static constexpr float VoltEmpty = 3600.0f; //[mV]
	static constexpr float EmaA = 0.05f;
	static constexpr float EmaA_sleep = 0.5f;

#if CONFIG_VERSION == 0x0007
	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 0;
	static constexpr int CalReads = 10;
	static constexpr float CalExpected = 2500;

	ADC& adc;
	PinOut refSwitch;
#endif

	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

#if CONFIG_VERSION == 0x0007
	std::unique_ptr<ADCReader> readerRef;
	adc_cali_handle_t caliRef;
#endif

	std::mutex mut;

	TimeHysteresis<ChargingState> chargeHyst;
	ChargingState lastCharging = ChargingState::Unplugged;
	bool sleep = false;

	std::atomic_bool abortFlag = false;

	SemaphoreHandle_t sem;
	Timer timer;
	static void isr(void* arg);
	void loop() override;

	void checkCharging(bool fresh = false);

#if CONFIG_VERSION == 0x0007
	void calibrate();
#endif

	void sample(bool fresh = false);
	void startTimer();

	bool shutdown = false;

#if CONFIG_VERSION == 0x0003
	static constexpr esp_efuse_desc_t adc1_low = { EFUSE_BLK3, 0, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_low[] = { &adc1_low, nullptr };
	static constexpr esp_efuse_desc_t adc1_high = { EFUSE_BLK3, 8, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_high[] = { &adc1_high, nullptr };
#endif
};

#endif //CLOCKSTAR_LIBRARY_BATTERY_H
