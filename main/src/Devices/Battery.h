#ifndef CLOCKSTAR_LIBRARY_BATTERY_H
#define CLOCKSTAR_LIBRARY_BATTERY_H

#include <hal/gpio_types.h>
#include <atomic>
#include "Util/Threaded.h"
#include "Periph/ADC.h"
#include "Util/Hysteresis.h"
#include "Periph/Timer.h"
#include "Util/TimeHysteresis.h"
#include "Services/ADCReader.h"
#include <mutex>
#include <esp_efuse.h>
#include <memory>

class Battery : private Threaded {
public:
	Battery(ADC& adc);
	~Battery() override;
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

	static constexpr uint32_t VoltFull = 4200; //[mV]
	static constexpr uint32_t VoltEmpty = 3600; //[mV]
	static constexpr float EMA_factor = 0.05;
	static constexpr float EMA_factor_sleep = 0.5;

	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

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
	void sample(bool fresh = false);
	void startTimer();

	bool shutdown = false;

	static constexpr esp_efuse_desc_t adc1_low = { EFUSE_BLK3, 0, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_low[] = { &adc1_low, nullptr };
	static constexpr esp_efuse_desc_t adc1_high = { EFUSE_BLK3, 8, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_high[] = { &adc1_high, nullptr };

};

#endif //CLOCKSTAR_LIBRARY_BATTERY_H
