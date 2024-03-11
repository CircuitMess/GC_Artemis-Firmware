#ifndef ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H
#define ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H

#include "UIElements/BatteryElement.h"

class Theme9BatteryElement : public BatteryElement {
public:
	explicit Theme9BatteryElement(lv_obj_t* parent);

protected:
	virtual void updateChargingVisuals() override;
	virtual void updateLevelVisuals() override;
};

#endif //ARTEMIS_FIRMWARE_THEME9BATTERYELEMENT_H