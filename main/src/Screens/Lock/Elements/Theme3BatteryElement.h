#ifndef ARTEMIS_FIRMWARE_THEME3BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME3BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme3BatteryElement : public BatteryElement {
public:
	explicit Theme3BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;
	virtual void loop() override;

private:
	lv_obj_t* batteryPercent;
	lv_obj_t* batteryImg;
	lv_obj_t* chargingImg;
};

#endif //ARTEMIS_FIRMWARE_THEME3BATTERYELEMENT_H