#include "NotifIconsElement.h"

NotifIconsElement::NotifIconsElement(lv_obj_t* parent, uint8_t maxIcons) : LVObject(parent), maxIconsCount(maxIcons){
	notifIcons.reserve(maxIconsCount);
}

void NotifIconsElement::clear(){
	notifIcons.clear();
	lv_obj_clean(*this);
}

void NotifIconsElement::add(const char* path){
	if(path == nullptr){
		return;
	}

	if(notifIcons.count(path)){
		notifIcons[path].count++;
		return;
	}

	if(notifIcons.size() >= maxIconsCount) return;

	lv_obj_t* icon = lv_img_create(*this);
	lv_img_set_src(icon, path);

	NotifIcon notifIcon = { 1, icon };
	notifIcons.insert({ path, notifIcon });
}

void NotifIconsElement::remove(const char* path){
	if(path == nullptr){
		return;
	}

	auto pair = notifIcons.find(path);
	if(pair == notifIcons.end()) return;

	pair->second.count -= 1;
	if(pair->second.count > 0) return;

	lv_obj_del(pair->second.icon);
	notifIcons.erase(pair);
}
