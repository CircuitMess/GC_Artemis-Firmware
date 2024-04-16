#ifndef ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme9BatteryElement : public BatteryElement {
public:
	explicit Theme9BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;
	virtual void loop() override;

private:
	lv_obj_t* batteryPercent;
	lv_obj_t* chargingImg;
	lv_obj_t* clockImg;
};

#endif //ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H