#ifndef CLOCKSTAR_FIRMWARE_BATTERYELEMENT_H
#define CLOCKSTAR_FIRMWARE_BATTERYELEMENT_H

#include "LV_Interface/LVObject.h"
#include "Filepaths.hpp"
#include "Devices/Battery.h"
#include "Util/Events.h"
#include "Settings/Settings.h"

class BatteryElement : public LVObject {
public:
	explicit BatteryElement(lv_obj_t* parent);
	~BatteryElement() override;

	enum Level {
		Empty, Low, Mid, Full, Charging
	};

	inline Level getLevel() const { return level; }

	virtual void loop();

	virtual void updateVisuals() override;

	virtual void updateChargingVisuals();
	virtual void updateLevelVisuals();

protected:
	uint8_t batteryLevels = 4;
	uint8_t chargingIndex = 0;
	Battery& battery;

protected:
	void set(Level level);

private:
	Level level = Full;
	EventQueue queue;

	static constexpr uint32_t ChargingAnimTime = 500;
	uint32_t chargingMillis = 0;
};


#endif //CLOCKSTAR_FIRMWARE_BATTERYELEMENT_H
