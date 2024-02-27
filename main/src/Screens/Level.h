#ifndef CLOCKSTAR_FIRMWARE_LEVEL_H
#define CLOCKSTAR_FIRMWARE_LEVEL_H

#include "../LV_Interface/LVScreen.h"
#include "../LV_Interface/LVStyle.h"
#include "../Util/EMA.h"
#include "../Devices/IMU.h"
#include "Util/Events.h"

class Level : public LVScreen {
public:
	Level();
	~Level() override;

	void setOrientation(double pitch, double roll);

private:
	lv_obj_t* bg;
	lv_obj_t* markingsHorizontal, * markingsCenter, * markingsVertical;
	lv_obj_t* bubbleHorizontal, * bubbleCenter, * bubbleVertical;

	struct Constraint {
		uint8_t min;
		uint8_t max;
	};

	IMU* imu;
	void loop() override;
	void onStart() override;
	void onStop() override;
	void onStarting() override;

	ThreadedClosure reader;
	void readerFunc();

	static constexpr TickType_t ReaderDelay = 40 / portTICK_PERIOD_MS;

	struct PitchRoll {
		double pitch;
		double roll;
	};
	Queue<PitchRoll> data;
	static constexpr int QueueSize = 10;

	EMA pitchFilter;
	EMA rollFilter;
	static constexpr float filterStrength = 0.2;

	EventQueue queue;

	static constexpr Constraint VerticalConstr = { 8, 68 };
	static constexpr Constraint HorizontalConstr = { 8, 68 };
	static constexpr int8_t CenterConstr = 29;
	static constexpr int8_t CenterPos = 38;
	static constexpr double AngleConstraint = 0.5f;

};

#endif //CLOCKSTAR_FIRMWARE_LEVEL_H
