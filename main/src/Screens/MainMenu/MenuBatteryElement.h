#ifndef ARTEMIS_FIRMWARE_MENUBATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_MENUBATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class MenuBatteryElement : public BatteryElement {
public:
	explicit MenuBatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;

private:
	lv_obj_t* img;
};

#endif //ARTEMIS_FIRMWARE_MENUBATTERYELEMENT_H