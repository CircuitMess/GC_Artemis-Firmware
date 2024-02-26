#include "LockScreen.h"
#include "Util/Services.h"
#include "Theme/theme.h"
#include "Services/Time.h"
#include "Util/stdafx.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/SleepMan.h"
#include "LV_Interface/FSLVGL.h"
#include "LV_Interface/InputLVGL.h"

LockScreen::LockScreen() : ts(*((Time*) Services.get(Service::Time))), phone(*((Phone*) Services.get(Service::Phone))), queue(24){
	notifs.reserve(MaxNotifs);
	notifIcons.reserve(MaxIconsCount);

	buildUI();

	lv_obj_add_event_cb(main, [](lv_event_t* evt){
		auto scr = static_cast<LockScreen*>(evt->user_data);
		scr->setSleep(true);
	}, LV_EVENT_FOCUSED, this);

	lv_obj_add_event_cb(main, [](lv_event_t* evt){
		auto scr = static_cast<LockScreen*>(evt->user_data);
		scr->setSleep(false);
		scr->locker->stop();
	}, LV_EVENT_DEFOCUSED, this);
}

void LockScreen::onStarting(){
	prepare();
}

void LockScreen::onStart(){
	queue.reset();
	Events::listen(Facility::Phone, &queue);
	Events::listen(Facility::Time, &queue);
	Events::listen(Facility::Input, &queue);
	setSleep(true);
}

void LockScreen::onStop(){
	Events::unlisten(&queue);
	setSleep(false);
}

void LockScreen::setSleep(bool en){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	if(!sleep) return;
	sleep->enAltLock(en);
}

void LockScreen::prepare(){
	lv_obj_scroll_to(rest, 0, 0, LV_ANIM_OFF);
	lv_obj_scroll_to(*this, 0, 0, LV_ANIM_OFF);
	lv_group_focus_obj(main);

	locker->hide();
	status->loop();
	clock->loop();
	updateTime(ts.getTime());
	queue.reset();
	updateNotifs();
}

void LockScreen::loop(){
	locker->loop();
	if(locker->t() >= 1){
		locker->loop();
		transition([](){ return std::make_unique<MainMenu>(); });
		return;
	}

	status->loop();

	clock->loop();

	if(millis() - lastTimeUpdate > TimeUpdateInterval){
		updateTime(ts.getTime());
	}

	Event evt;
	while(queue.get(evt, 0)){
		if(evt.facility == Facility::Phone){
			auto data = (Phone::Event*) evt.data;
			processEvt(*data);
		}else if(evt.facility == Facility::Time){
			auto data = (Time::Event*) evt.data;
			if(data->action == Time::Event::Updated){
				updateTime(data->updated.time);
			}
		}else if(evt.facility == Facility::Input){
			auto data = (Input::Data*) evt.data;
			processInput(*data);
		}
		free(evt.data);
	}
}

void LockScreen::processInput(const Input::Data& evt){
	if(evt.btn == Input::Alt && evt.action == Input::Data::Press && lv_group_get_focused(inputGroup) != main){
		lv_group_focus_obj(main);
		return;
	}

	if(lv_group_get_focused(inputGroup) != main) return;

	if(evt.btn == Input::Select){
		if(evt.action == Input::Data::Press){
			locker->start();
		}else if(evt.action == Input::Data::Release){
			bool hide = locker->t() < 0.05;
			locker->stop();
			if(hide){
				locker->hide();
			}
		}
	}
}

void LockScreen::processEvt(const Phone::Event& evt){
	if(evt.action == Phone::Event::Added || evt.action == Phone::Event::Changed){
		auto notif = phone.getNotif(evt.data.addChgRem.id);
		notifAdd(notif);
	}else if(evt.action == Phone::Event::Removed){
		notifRem(evt.data.addChgRem.id);
	}else if(evt.action == Phone::Event::Cleared){
		notifsClear();
	}
}

void LockScreen::updateNotifs(){
	const auto set = phone.getNotifs();
	auto find = [&set](uint32_t uid) -> const Notif*{
		for(auto& notif : set){
			if(notif.uid == uid) return &notif;
		}
		return nullptr;
	};

	std::unordered_set<uint32_t> forRem;
	for(const auto& pair : notifs){
		auto exists = find(pair.first);
		if(!exists){
			forRem.insert(pair.first);
		}
	}
	for(uint32_t uid : forRem){
		notifRem(uid);
	}

	for(const auto& notif : set){
		if(notifs.count(notif.uid) == 0){
			notifAdd(notif);
		}
	}
}

void LockScreen::notifAdd(const Notif& notif){
	bool isModify = true;

	if(notifs.count(notif.uid) == 0){
		if(notifs.size() >= MaxNotifs) return;

		isModify = false;

		auto uid = notif.uid;
		auto item = new Item(rest, [this, uid](){
			notifRem(uid);
			phone.doNeg(uid);
		});

		bool itemActive = false;
		if(InputLVGL::getInstance()->getIndev()->group != inputGroup){
			itemActive = true;
		}

		auto focused = lv_group_get_focused(inputGroup);
		lv_obj_move_to_index(*item, 0);
		lv_group_remove_all_objs(inputGroup);
		lv_group_add_obj(inputGroup, main);
		for(int j = 0; j < lv_obj_get_child_cnt(rest); ++j){
			lv_group_add_obj(inputGroup, lv_obj_get_child(rest, j));
		}

		lv_group_focus_obj(focused);
		if(itemActive){
			lv_event_send(focused, LV_EVENT_CLICKED, nullptr);
		}

		lv_obj_add_flag(*item, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
		lv_obj_add_flag(*item, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

		notifs.insert(std::make_pair(notif.uid, item));

		addNotifIcon(iconPath(notif));
	}

	auto el = notifs[notif.uid];

	if(isModify){
		removeNotifIcon(el->iconPath());
		addNotifIcon(iconPath(notif));
	}
	el->update(notif);
}

void LockScreen::notifRem(uint32_t id){
	auto it = notifs.find(id);
	if(it == notifs.end()) return;
	auto& el = it->second;

	removeNotifIcon(el->iconPath());
	lv_group_focus_next(inputGroup);

	lv_obj_del(*el);
	notifs.erase(it);
}

void LockScreen::notifsClear(){
	notifs.clear(); // This has to precede rest clearing
	lv_obj_clean(rest);

	notifIcons.clear();
	lv_obj_clean(icons);
}


void LockScreen::addNotifIcon(const char* path){
	if(path == nullptr) return;

	if(notifIcons.count(path)){
		notifIcons[path].count++;
		return;
	}

	if(notifIcons.size() >= MaxIconsCount) return;


	lv_obj_t* icon = lv_img_create(icons);
	lv_img_set_src(icon, path);

	NotifIcon notifIcon = { 1, icon };
	notifIcons.insert({ path, notifIcon });
}

void LockScreen::removeNotifIcon(const char* path){
	if(path == nullptr) return;

	auto pair = notifIcons.find(path);
	if(pair == notifIcons.end()) return;

	pair->second.count -= 1;
	if(pair->second.count > 0) return;

	lv_obj_del(pair->second.icon);
	notifIcons.erase(pair);
}

void LockScreen::updateTime(const tm& time){
	lastTimeUpdate = millis();

	// Update any date/time in UI here

	/*char dateText[128];

	static const char* Months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	const int dayLd = time.tm_mday - (time.tm_mday / 10) * 10;
	const char* daySuff;
	if(time.tm_mday == 11 || time.tm_mday == 12) daySuff = "th";
	else if(dayLd == 1) daySuff = "st";
	else if(dayLd == 2) daySuff = "nd";
	else if(dayLd == 3) daySuff = "rd";
	else daySuff = "th";

	snprintf(dateText, sizeof(dateText), "%s %d%s, %d", Months[time.tm_mon % 12], time.tm_mday, daySuff, 1900 + time.tm_year);*/
}

void LockScreen::buildUI(){
	lv_obj_add_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);

	main = lv_obj_create(*this);
	static const lv_coord_t col_dsc[] = { 128, LV_GRID_TEMPLATE_LAST };
	static const lv_coord_t row_dsc[] = { 15, 17, 50, 29, 17, LV_GRID_TEMPLATE_LAST };
	lv_obj_set_style_grid_column_dsc_array(main, col_dsc, 0);
	lv_obj_set_style_grid_row_dsc_array(main, row_dsc, 0);
	lv_obj_set_size(main, 128, 128);
	lv_obj_set_layout(main, LV_LAYOUT_GRID);


	status = new StatusBar(main, false);
	lv_obj_set_grid_cell(status->operator lv_obj_t*(), LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

	clock = new ClockLabelBig(main);
	lv_obj_set_grid_cell(clock->operator lv_obj_t*(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);

	icons = lv_obj_create(main);
	lv_obj_set_size(icons, 128, 11);
	lv_obj_set_style_min_height(icons, 1, 0);
	lv_obj_set_flex_flow(icons, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(icons, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(icons, 2, 0);
	lv_obj_set_grid_cell(icons, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);

	locker = new Slider(main);
	lv_obj_set_grid_cell(locker->operator lv_obj_t*(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);

	rest = lv_obj_create(*this);
	lv_obj_set_size(rest, 128, 128);
	lv_obj_set_flex_flow(rest, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(rest, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_hor(rest, 4, 0);
	lv_obj_set_style_pad_ver(rest, 3, 0);
	lv_obj_set_style_pad_gap(rest, 3, 0);

	// BG

	lv_obj_set_style_bg_color(main, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(main, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(main, "S:/lockbg.bin", 0);

	lv_obj_set_style_bg_color(rest, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(rest, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(rest, lv_color_make(255, 101, 0), 0);
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
