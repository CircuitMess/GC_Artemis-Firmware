#ifndef ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H
#define ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H

#include <unordered_map>
#include <string>
#include "LV_Interface/LVObject.h"
#include "Notifs/Notif.h"

class NotifIconsElement  : public LVObject {
public:
	explicit NotifIconsElement(lv_obj_t* parent, uint8_t maxIcons = 20);

	void clear();
	void add(NotifIcon icon);
	void remove(NotifIcon icon);

protected:
	struct NotifIconCount {
		uint32_t count;
		lv_obj_t* icon;
	};

	std::unordered_map<NotifIcon, NotifIconCount> notifIcons;
	uint8_t maxIconsCount = 20;
};

#endif //ARTEMIS_FIRMWARE_NOTIFICONSELEMENT_H