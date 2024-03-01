#ifndef CLOCKSTAR_FIRMWARE_THEREMIN_H
#define CLOCKSTAR_FIRMWARE_THEREMIN_H

#include <atomic>
#include "LV_Interface/LVScreen.h"
#include "LV_Interface/LVStyle.h"
#include "ArpeggioSequence.h"
#include "Services/ChirpSystem.h"
#include "Util/Queue.h"
#include "Util/EMA.h"
#include "Devices/IMU.h"
#include "Util/Events.h"

class Theremin : public LVScreen {
public:
	Theremin();
	~Theremin() override;

	void setOrientation(float pitch, float roll);

private:
	void onStart() override;
	void onStop() override;
	void loop() override;

	lv_obj_t* bg;
	lv_obj_t* sliderVertical, * sliderHorizontal;

	static constexpr float AngleConstraint = 0.5f;
	static constexpr uint8_t SliderLength = 97;
	static constexpr uint8_t SliderWidth = 10;
	static constexpr uint8_t SliderRange = 92;


	static constexpr uint8_t VerticalBarX = 7;
	static constexpr uint8_t VerticalBarY = 7;
	static constexpr uint8_t VerticalTextX = 18;
	static constexpr uint8_t VerticalTextY = 5;

	static constexpr uint8_t HorizontalBarX = 24;
	static constexpr uint8_t HorizontalBarY = 111;
	static constexpr uint8_t HorizontalTextX = 33;
	static constexpr uint8_t HorizontalTextY = 69;

	ChirpSystem& audio;
	ArpeggioSequence sequence;


	static constexpr uint32_t DRAM_ATTR SequenceDuration = 800; //ms
	static constexpr uint32_t getToneDuration(uint8_t sequenceSize);

	SemaphoreHandle_t sem;
	Timer timer;
	static void timerCB(void* arg);

	ThreadedClosure audioThread;
	void audioThreadFunc();
	volatile uint8_t sequenceIndex = 0;


	IMU* imu;
	struct PitchRoll {
		float pitch;
		float roll;
	};
	EMA pitchFilter;
	EMA rollFilter;
	static constexpr float filterStrength = 0.12;

	EventQueue queue;
	std::atomic_bool abortFlag = false;

	void buildUI();
};


#endif //CLOCKSTAR_FIRMWARE_THEREMIN_H
