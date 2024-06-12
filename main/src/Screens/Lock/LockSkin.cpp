#include "LockSkin.h"
#include "Filepaths.hpp"
#include "Settings/Settings.h"
#include "Util/Services.h"
#include "LV_Interface/InputLVGL.h"
#include "Screens/Lock/Elements/Theme2BatteryElement.h"
#include "Screens/Lock/Elements/Theme3BatteryElement.h"
#include "Screens/Lock/Elements/Theme4BatteryElement.h"
#include "Screens/Lock/Elements/Theme5BatteryElement.h"
#include "Screens/Lock/Elements/Theme6BatteryElement.h"
#include "Screens/Lock/Elements/Theme9BatteryElement.h"
#include "Screens/Lock/Elements/Theme7BatteryElement.h"
#include "Screens/Lock/Elements/Theme8BatteryElement.h"
#include "UIElements/NotifIconsElement.h"
#include "Theme/theme.h"

LockSkin::LockSkin(lv_obj_t* parent, lv_group_t* inputGroup) : LVObject(parent), inputGroup(inputGroup){
	notifs.reserve(MaxNotifs);
}

void LockSkin::loop(){
	if(locker != nullptr){
		locker->loop();
		if(locker->t() >= 1){
			locker->loop();
			return;
		}
	}

	if(phoneElement != nullptr){
		phoneElement->loop();
	}

	if(batteryElement != nullptr){
		batteryElement->loop();
	}

	if(clock != nullptr){
		clock->loop();
	}

	setDateLabel();
}

void LockSkin::prepare(){
	buildUI();

	lv_obj_scroll_to(rest, 0, 0, LV_ANIM_OFF);
	lv_obj_scroll_to(*this, 0, 0, LV_ANIM_OFF);
	lv_group_focus_obj(main);

	if(locker != nullptr){
		locker->hide();
	}

	if(phoneElement != nullptr){
		phoneElement->loop();
	}

	if(batteryElement != nullptr){
		batteryElement->loop();
	}

	if(clock != nullptr){
		clock->loop();
	}

	updateNotifs();
}

void LockSkin::notifAdd(const Notif& notif){
	bool isModify = true;

	if(notifs.count(notif.uid) == 0){
		if(notifs.size() >= MaxNotifs) return;

		isModify = false;

		auto uid = notif.uid;
		auto item = new Item(notifList, [this, uid](){
			notifRem(uid);

			if(Phone* phone = (Phone*) Services.get(Service::Phone)){
				phone->doNeg(uid);
			}
		});

		bool itemActive = false;
		if(InputLVGL::getInstance()->getIndev()->group != inputGroup){
			itemActive = true;
		}

		auto focused = lv_group_get_focused(inputGroup);
		lv_obj_move_to_index(*item, 0);
		lv_group_remove_all_objs(inputGroup);
		lv_group_add_obj(inputGroup, getMain());
		for(int j = 0; j < lv_obj_get_child_cnt(notifList); ++j){
			lv_group_add_obj(inputGroup, lv_obj_get_child(notifList, j));
		}

		lv_group_focus_obj(focused);
		if(itemActive){
			lv_event_send(focused, LV_EVENT_CLICKED, nullptr);
		}

		lv_obj_add_flag(*item, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
		lv_obj_add_flag(*item, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

		notifs.insert(std::make_pair(notif.uid, item));

		addNotifIcon(notifIcon(notif));
	}

	auto el = notifs[notif.uid];

	if(isModify){
		removeNotifIcon(el->getNotifIcon());
		addNotifIcon(notifIcon(notif));
	}
	el->update(notif);
}

void LockSkin::notifRem(uint32_t id){
	auto it = notifs.find(id);
	if(it == notifs.end()) return;
	auto& el = it->second;

	removeNotifIcon(el->getNotifIcon());
	lv_group_focus_next(inputGroup);

	lv_obj_del(*el);
	notifs.erase(it);
}

void LockSkin::notifsClear(){
	notifs.clear(); // This has to precede rest clearing
	lv_obj_clean(notifList);

	if(icons == nullptr){
		return;
	}

	icons->clear();
}

void LockSkin::addNotifIcon(NotifIcon icon){
	if(icons == nullptr) return;

	icons->add(icon);
}

void LockSkin::removeNotifIcon(NotifIcon icon){
	if(icons == nullptr) return;

	icons->remove(icon);
}

void LockSkin::updateNotifs(){
	Phone* phone = (Phone*) Services.get(Service::Phone);
	if(phone == nullptr){
		return;
	}

	const auto set = phone->getNotifs();
	auto find = [&set](uint32_t uid) -> const Notif*{
		for(auto& notif: set){
			if(notif.uid == uid) return &notif;
		}
		return nullptr;
	};

	std::unordered_set<uint32_t> forRem;
	for(const auto& pair: notifs){
		auto exists = find(pair.first);
		if(!exists){
			forRem.insert(pair.first);
		}
	}
	for(uint32_t uid: forRem){
		notifRem(uid);
	}

	for(const auto& notif: set){
		if(notifs.count(notif.uid) == 0){
			notifAdd(notif);
		}
	}
}

void LockSkin::buildUI(){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const ThemeStruct themeData = settings->get().themeData;

	lv_obj_add_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);

	main = lv_obj_create(*this);
	if(main == nullptr){
		return;
	}

	lv_obj_set_size(main, 128, 128);

	icons = new NotifIconsElement(main, themeData.notifData.maxNotifs);
	lv_obj_set_x(*icons, themeData.notifData.x);
	lv_obj_set_y(*icons, themeData.notifData.y);
	lv_obj_set_style_min_height(*icons, 1, 0);
	lv_obj_set_size(*icons, themeData.notifData.w, themeData.notifData.h);
	lv_obj_set_align(*icons, themeData.notifData.align);
	lv_obj_set_flex_flow(*icons, themeData.notifData.flex);
	lv_obj_set_flex_align(*icons, themeData.notifData.mainAlign, themeData.notifData.crossAlign, themeData.notifData.trackAlign);
	lv_obj_set_style_pad_gap(*icons, themeData.notifData.gapPad, 0);

	phoneElement = new PhoneElement(main, false, themeData.specialPhone);

	switch(themeData.theme){
		case Theme::Theme1:{
			batteryElement = new MenuBatteryElement(main);
			break;
		}
		case Theme::Theme2:{
			batteryElement = new Theme2BatteryElement(main);
			break;
		}
		case Theme::Theme3:{
			batteryElement = new Theme3BatteryElement(main);
			break;
		}
		case Theme::Theme4:{
			batteryElement = new Theme4BatteryElement(main);
			break;
		}
		case Theme::Theme5:{
			batteryElement = new Theme5BatteryElement(main);
			break;
		}
		case Theme::Theme6:{
			batteryElement = new Theme6BatteryElement(main);
			break;
		}
		case Theme::Theme7:{
			batteryElement = new Theme7BatteryElement(main);
			break;
		}
		case Theme::Theme8:{
			batteryElement = new Theme8BatteryElement(main);
			break;
		}
		case Theme::Theme9:{
			batteryElement = new Theme9BatteryElement(main);
			break;
		}
		default:{
			break;
		}
	}

	lv_obj_set_align(*batteryElement, LV_ALIGN_TOP_LEFT);
	lv_obj_set_x(*batteryElement, themeData.batteryX);
	lv_obj_set_y(*batteryElement, themeData.batteryY);

	lv_obj_set_align(*phoneElement, LV_ALIGN_CENTER);
	lv_obj_set_x(*phoneElement, themeData.phoneX);
	lv_obj_set_y(*phoneElement, themeData.phoneY);

	clock = new ClockLabelBig(main, themeData.verticalClock, themeData.clockPadding);
	lv_obj_set_align(*clock, LV_ALIGN_CENTER);
	lv_obj_set_x(*clock, themeData.clockX);
	lv_obj_set_y(*clock, themeData.clockY);

	date = lv_label_create(main);
	if(date == nullptr){
		return;
	}

	lv_obj_set_style_text_font(date, &landerfont, 0);
	lv_obj_set_align(date, LV_ALIGN_CENTER);
	lv_obj_set_x(date, themeData.dateX);
	lv_obj_set_y(date, themeData.dateY);
	lv_obj_set_style_text_color(date, themeData.dateColor, 0);
	setDateLabel();

	locker = new Slider(main, themeData.sliderConfig);
	lv_obj_set_y(*locker, themeData.sliderY);

	rest = lv_obj_create(*this);
	if(rest == nullptr){
		return;
	}

	lv_obj_set_size(rest, 128, 128);

	notifList = lv_obj_create(rest);
	if(notifList == nullptr){
		return;
	}

	lv_obj_set_size(notifList, 128, 126);
	lv_obj_set_align(notifList, LV_ALIGN_CENTER);
	lv_obj_set_flex_flow(notifList, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(notifList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_hor(notifList, 4, 0);
	lv_obj_set_style_pad_ver(notifList, 3, 0);
	lv_obj_set_style_pad_gap(notifList, 3, 0);

	// BG

	lv_obj_set_style_bg_color(main, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(main, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(main, THEMED_FILE(LockScreen, Background, themeData.theme), 0);

	lv_obj_set_style_bg_color(rest, settings->get().themeData.backgroundColor, 0);
	lv_obj_set_style_bg_opa(rest, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(rest, settings->get().themeData.highlightColor, 0);
	lv_obj_set_style_border_opa(rest, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(rest, 1, 0);

	// Scrolling

	lv_obj_add_flag(*this, LV_OBJ_FLAG_SCROLL_ONE);
	lv_obj_set_scroll_snap_y(*this, LV_SCROLL_SNAP_START);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SNAPPABLE);
	lv_obj_add_flag(rest, LV_OBJ_FLAG_SNAPPABLE);

	lv_group_add_obj(inputGroup, main);
	lv_group_set_wrap(inputGroup, false);
	lv_obj_add_flag(main, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_clear_flag(main, LV_OBJ_FLAG_SCROLLABLE);
}

void LockSkin::setDateLabel(){
	if(date == nullptr){
		return;
	}

	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Time* time = (Time*) Services.get(Service::Time);
	if(time == nullptr){
		return;
	}

	const tm tm = time->getTime();

	constexpr static const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

	const std::string day = std::to_string(tm.tm_mday) +
							(tm.tm_mday % 10 == 1 && tm.tm_mday / 10 != 1 ? "ST" :
							 tm.tm_mday % 10 == 2 && tm.tm_mday / 10 != 1 ? "ND" :
							 tm.tm_mday % 10 == 3 && tm.tm_mday / 10 != 1 ? "RD" : "TH");
	const std::string month = months[tm.tm_mon];

	const std::string dateString = (settings->get().dateFormat == DateFormat::Regular ? day : month) + " " +
								   (settings->get().dateFormat == DateFormat::Regular ? month : day) + " " + std::to_string(tm.tm_year + 1900);

	lv_label_set_text(date, dateString.c_str());
}
