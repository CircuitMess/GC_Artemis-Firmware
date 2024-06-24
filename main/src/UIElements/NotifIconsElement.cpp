#include "NotifIconsElement.h"

NotifIconsElement::NotifIconsElement(lv_obj_t* parent, uint8_t maxIcons) : LVObject(parent), maxIconsCount(maxIcons){
	notifIcons.reserve(maxIconsCount);
}

void NotifIconsElement::clear(){
	notifIcons.clear();
	lv_obj_clean(*this);
}

void NotifIconsElement::add(NotifIcon icon){
	if(notifIcons.count(icon)){
		notifIcons[icon].count++;
		return;
	}

	if(notifIcons.size() >= maxIconsCount) return;

	lv_obj_t* img = lv_img_create(*this);
	lv_img_set_src(img, iconPath(icon));

	NotifIconCount notifIcon = { 1, img };
	notifIcons.insert({ icon, notifIcon });
}

void NotifIconsElement::remove(NotifIcon icon){
	auto pair = notifIcons.find(icon);
	if(pair == notifIcons.end()) return;

	pair->second.count -= 1;
	if(pair->second.count > 0) return;

	lv_obj_del(pair->second.icon);
	notifIcons.erase(pair);
}
