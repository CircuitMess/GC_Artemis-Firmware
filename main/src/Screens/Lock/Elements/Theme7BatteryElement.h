#ifndef ARTEMIS_FIRMWARE_THEME7BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME7BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme7BatteryElement : public BatteryElement {
public:
	explicit Theme7BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;
	virtual void loop() override;

private:
	lv_obj_t* lvl1;
	lv_obj_t* lvl2;
	lv_obj_t* lvl3;
	lv_obj_t* lvl4;
};

#endif //ARTEMIS_FIRMWARE_THEME7BATTERYELEMENT_H