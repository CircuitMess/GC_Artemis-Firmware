#ifndef ARTEMIS_FIRMWARE_THEME5BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME5BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme5BatteryElement : public BatteryElement {
public:
	explicit Theme5BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;
	virtual void loop() override;

private:
	lv_obj_t* batteryIndicator;
};

#endif //ARTEMIS_FIRMWARE_THEME5BATTERYELEMENT_H