#ifndef CLOCKSTAR_FIRMWARE_IMU_H
#define CLOCKSTAR_FIRMWARE_IMU_H

#include "Drivers/lsm6ds3tr-c_reg.h"
#include "Periph/I2C.h"
#include "Util/Threaded.h"
#include "Util/Queue.h"
#include <cstdint>
#include <vector>
#include <memory>

/**
 * Axis orientation when the watch is on your left wrist:
 * 		X - from device into the direction of your fingers
 * 		Y - towards the lower edge of the device
 * 		Z - from the screen into the air
 */

class IMU {
public:
	IMU(I2C& i2c);
	virtual ~IMU();

	enum class TiltDirection {
		Lifted, Lowered
	};
	enum class HandSide {
		Left, Right
	};
	enum class WatchPosition {
		FaceUp, FaceDown
	};

	struct Event {
		enum { SignMotion, SingleTap, WristTilt, FIFO, DoubleTap } action;
		union {
			TiltDirection wristTiltDir;
		};
	};

	// Linear acceleration is in m/s^2, angular velocity is in rad/s
	struct Sample {
		double gyroX;
		double gyroY;
		double gyroZ;
		double accelX;
		double accelY;
		double accelZ;
	};

	/**
	 * Sets the tilt direction that should trigger tilt detection. Default is lifted.
	 * @param direction Lifting it towards you or lowering it away from you
	 */
	void setTiltDirection(TiltDirection direction);

	void enableTiltDetection(bool enable);

	Sample getSample();

	void clearSources();

	void shutdown();

	bool init();

private:
	static constexpr uint8_t Addr = 0x6A;
	I2C& i2c;

	/**
	 * !DELETED FUNCTION!
	 * Enable/Disable significant motion detection events. OFF by default
	 * @param enable
	 */
	void enableMotionDetection(bool enable);

	/**
	 * !DELETED FUNCTION!
	 */
	bool pollFIFO(Sample& sample, TickType_t wait = portMAX_DELAY);

	/**
	 * !DELETED FUNCTION!
	 */
	void enableFIFO(bool enable);

	/**
	 * Sets the watch wear position for tilt detection. Default is face-up.
	 * @param wristPosition Face-up or face-down position
	 */
	void setWristPosition(WatchPosition wristPosition);

	static int32_t platform_write(void* hndl, uint8_t reg, const uint8_t* data, uint16_t len);
	static int32_t platform_read(void* hndl, uint8_t reg, uint8_t* data, uint16_t len);
	stmdev_ctx_t ctx = {
			.write_reg = platform_write,
			.read_reg = platform_read,
			.mdelay = vTaskDelay,
			.handle = this
	};

	static constexpr uint16_t ReadingsWatermark = 200;
	static constexpr size_t MaxReads = 250;
	Queue<Sample> fifoSamples;
	void clearFifo();

	bool tiltEnable = true;
	TiltDirection tiltDirection = TiltDirection::Lifted;
	HandSide handSide = HandSide::Left;
	WatchPosition position = WatchPosition::FaceUp;
	static constexpr uint8_t SignificantMotionSens = 4;

	void printInterruptInfo();

	SemaphoreHandle_t sem1 = nullptr;
	SemaphoreHandle_t sem2 = nullptr;
	static void IRAM_ATTR isr1(void* arg);
	static void IRAM_ATTR isr2(void* arg);
	ThreadedClosure thread1;
	ThreadedClosure thread2;
	void thread1Func();
	void thread2Func();

	void fetchEvents();

	static double xlConv(int16_t raw);
	static double gyConv(int16_t raw);
};


#endif //CLOCKSTAR_FIRMWARE_IMU_H
