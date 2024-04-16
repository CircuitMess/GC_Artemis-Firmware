#ifndef ARTEMIS_FIRMWARE_THEME2BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME2BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme2BatteryElement : public BatteryElement {
public:
	explicit Theme2BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;

private:
	lv_obj_t* main;
	lv_obj_t* clockImg;
	lv_obj_t* lvl1;
	lv_obj_t* lvl2;
	lv_obj_t* lvl3;
};

#endif //ARTEMIS_FIRMWARE_THEME2BATTERYELEMENT_H