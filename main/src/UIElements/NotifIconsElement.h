#ifndef ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H
#define ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H

#include <unordered_map>
#include "LV_Interface/LVObject.h"

class NotifIconsElement  : public LVObject {
public:
	explicit NotifIconsElement(lv_obj_t* parent, uint8_t maxIcons = 20);

	void clear();
	void add(const char* path);
	void remove(const char* path);

protected:
	struct NotifIcon {
		uint32_t count;
		lv_obj_t* icon;
	};

	std::unordered_map<const char*, NotifIcon> notifIcons;
	uint8_t maxIconsCount = 20;
};

#endif //ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H