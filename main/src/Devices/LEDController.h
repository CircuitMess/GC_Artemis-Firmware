#ifndef BATMOBILE_FIRMWARE_LEDCONTROLLER_H
#define BATMOBILE_FIRMWARE_LEDCONTROLLER_H

#include <glm.hpp>
#include <vector>
#include <mutex>
#include <atomic>
#include "Util/Threaded.h"
#include "Periph/PWM.h"
#include "Periph/Timer.h"
#include "Util/SleepLock.h"
#include "Periph/PinOut.h"

template <typename T>
class LEDController : private Threaded {
public:
	LEDController();

	~LEDController() override;

	void begin();
	void end();

	void clear();
	void setSolid(T color);
	void blink(T color, uint32_t onTime = DefaultBlinkDuration, uint32_t offTime = DefaultBlinkDuration);
	void blinkTwice(T color, uint32_t onTime = DefaultBlinkDuration, uint32_t offTime = DefaultBlinkDuration);
	/**
	 * Starts a continuous blinking in the selected color.
	 * @param color Selected color
	 * @param duration Duration of LED on/off cycle. LED will stay the same color for 'duration' ms, then turn off for 'duration' ms.
	 */
	void blinkContinuous(T color, int32_t loops = -1, uint32_t onTime = 500, uint32_t offTime = 500);

	/**
	 * Breathes an LED from specified start value to end value.
	 * Typename T needs to have defined subtraction (T start - T end) and multiplication by a scalar value (T start * float n)
	 * @param start Starting value
	 * @param end Ending value
	 * @param period Duration of 1 period, going from start to end and back, in milliseconds.
	 * @param loops Number of loops, -1 for infinite looping.
	 */
	void breathe(T start, T end, size_t period, int32_t loops = -1);

protected:
	virtual void init() = 0;
	virtual void deinit() = 0;
	virtual void write(T val) = 0;

private:
	void loop() override;

	struct ShortAction {
		enum {
			BlinkOnce, BlinkTwice, None
		} type;
		enum {
			Pending, On, Off
		} state;

		uint32_t onTime;
		uint32_t offTime;
		T color;
	} shortAction;

	struct ContinuousAction {
		enum {
			Solid, ContinuousBlink, Breathe, None
		} type;
		enum {
			Pending, On, Off
		} state;
		union {
			T solidColor;

			struct {
				T color;
				uint32_t onTime;
				uint32_t offTime;
				int32_t loops;
				uint32_t currLoops;

			} continuousBlink;

			struct {
				T start, end;
				uint32_t period;
				int32_t loops;
				uint32_t currLoops;
				uint32_t t;
			} breathe;
		} data;
	} continuousAction;

	SleepLock sleepLock;

	SemaphoreHandle_t timerSem;
	Timer timer;

	uint32_t handleShortAction();
	uint32_t handleContinuousAction();

	std::mutex mut;
	std::atomic_bool abortFlag = false;

	static constexpr uint32_t DefaultBlinkDuration = 100; //[ms]
	static constexpr uint32_t BreatheDeltaT = 20;
};


class SingleLEDController : public LEDController<uint8_t> {
public:
	explicit SingleLEDController(PWM& pwm);

protected:
	void write(uint8_t val) override;

	void init() override;
	void deinit() override;

private:
	PWM& pwm;
};

class DigitalLEDController : public LEDController<uint8_t> {
public:
	explicit DigitalLEDController(gpio_num_t pin, bool inverted = false);

protected:
	void write(uint8_t val) override;

	void init() override;
	void deinit() override;

private:
	PinOut led;
};

//RGBLEDController template type is vector of floats to allow multiplication by a float value, this is a limitation set by glm
class RGBLEDController : public LEDController<glm::vec3> {
public:
	RGBLEDController(PWM& pwmR, PWM& pwmG, PWM& pwmB);

protected:
	void write(glm::vec3 val) override;

	void init() override;
	void deinit() override;

private:
	PWM& pwmR;
	PWM& pwmG;
	PWM& pwmB;
};


#endif //BATMOBILE_FIRMWARE_LEDCONTROLLER_H
