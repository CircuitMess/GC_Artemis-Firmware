#ifndef ARTEMIS_FIRMWARE_THEME4BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME4BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme4BatteryElement : public BatteryElement {
public:
	explicit Theme4BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;

private:
	lv_obj_t* clockIcon;
	lv_obj_t* batteryIcon;
};

#endif //ARTEMIS_FIRMWARE_THEME4BATTERYELEMENT_H