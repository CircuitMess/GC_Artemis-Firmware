#ifndef CLOCKSTAR_LIBRARY_BATTERY_H
#define CLOCKSTAR_LIBRARY_BATTERY_H

#include <atomic>
#include "Util/Threaded.h"
#include "Periph/ADC.h"
#include "Periph/Timer.h"
#include "Util/TimeHysteresis.h"
#include <mutex>
#include <memory>

class Battery : public Threaded {
public:
	Battery();
	virtual ~Battery() override;
	void begin();

	enum Level { Critical = 0, VeryLow, Low, Mid, Full, COUNT };
	enum class ChargingState : uint8_t { Unplugged, Charging, Full };

	virtual uint8_t getPerc() const = 0;
	virtual Level getLevel() const = 0;
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
	void setShutdown(bool value) { shutdown = value; }

	void setSleep(bool sleep);

protected:
	void loop() override;

	void checkCharging(bool fresh = false);

	virtual void sample(bool fresh = false) = 0;
	virtual void onSleep(bool sleep) {}
	void startTimer();

private:
	static constexpr uint32_t ShortMeasureIntverval = 100;
	static constexpr uint32_t LongMeasureIntverval = 6000;

	std::mutex mut;

	TimeHysteresis<ChargingState> chargeHyst;
	ChargingState lastCharging = ChargingState::Unplugged;
	bool sleep = false;

	std::atomic_bool abortFlag = false;

	SemaphoreHandle_t sem;
	Timer timer;
	static void isr(void* arg);

	bool shutdown = false;
};

#endif //CLOCKSTAR_LIBRARY_BATTERY_H
