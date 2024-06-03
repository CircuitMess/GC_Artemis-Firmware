#include "MainMenu.h"
#include "Util/Services.h"
#include "MenuItemAlt.h"
#include "Screens/Lock/LockScreen.h"
#include "Screens/Level.h"
#include "Screens/Theremin/Theremin.h"
#include "Screens/PerseCtrl/PerseCtrlScreen.h"
#include "Screens/Settings/SettingsScreen.h"
#include "Util/stdafx.h"
#include "LV_Interface/InputLVGL.h"
#include "Screens/Lander/LunarLander.h"
#include "Services/StatusCenter.h"

uint8_t  MainMenu::lastIndex = UINT8_MAX;

MainMenu::MainMenu() : phone(*((Phone*) Services.get(Service::Phone))), queue(4){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const auto theme = settings->get().themeData.theme;
	setupItemPaths(theme);

	lv_img_cache_set_size(8);

	lv_obj_set_size(*this, 128, LV_SIZE_CONTENT);
	lv_obj_add_flag(*this, LV_OBJ_FLAG_SCROLLABLE);

	bg = lv_obj_create(*this);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(bg, THEMED_FILE(Menu, Background, theme), 0);

	static constexpr auto statusBarHeight = 15;
	container = lv_obj_create(*this);
	lv_obj_set_pos(container, 0, statusBarHeight + (theme == Theme::Theme9 ? 5 : 0));
	lv_obj_set_style_pad_hor(container, 1, 0);
	lv_obj_set_size(container, 128, lv_obj_get_height(*this) - statusBarHeight - (theme == Theme::Theme9 ? 5 : 0));
	lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


	for(int i = 0; i < ItemCount; i++){
		const auto& info = ItemInfos[i];

		MenuItem* item;
		if(info.iconAltPath || info.labelTextAlt){
			auto itemAlt = new MenuItemAlt(container, info.iconPath, info.labelText);
			itemAlt->setAltParams(info.iconAltPath, info.labelTextAlt);
			item = itemAlt;
		}else{
			item = new MenuItem(container, info.iconPath, info.labelText);
		}

		lv_obj_add_event_cb(*item, [](lv_event_t* evt){
			auto menu = static_cast<MainMenu*>(evt->user_data);
			menu->onClick();
		}, LV_EVENT_CLICKED, this);

		lv_group_add_obj(inputGroup, *item);
		lv_obj_add_flag(*item, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
		lv_obj_add_flag(*item, LV_OBJ_FLAG_SNAPPABLE);
		lv_obj_clear_flag(*item, LV_OBJ_FLAG_CLICK_FOCUSABLE);

		items[i] = item;

		lv_obj_add_event_cb(*item, [](lv_event_t* evt){
			auto menu = static_cast<MainMenu*>(evt->user_data);

			const auto index = lv_obj_get_index(evt->target);
			menu->lastDefocus = index;
		}, LV_EVENT_DEFOCUSED, this);

		lv_obj_add_event_cb(*item, [](lv_event_t* evt){
			auto menu = static_cast<MainMenu*>(evt->user_data);

			const auto index = lv_obj_get_index(evt->target);

			const auto status = (StatusCenter*) Services.get(Service::Status);
			if(index > menu->lastDefocus){
				status->blinkDown();
			}else if(index < menu->lastDefocus){
				status->blinkUp();
			}

		}, LV_EVENT_FOCUSED, this);
	}

	//find my phone
	lv_obj_add_event_cb(*items[0], [](lv_event_t* evt){
		auto menu = static_cast<MainMenu*>(evt->user_data);
		if(menu->findPhoneRinging){
			menu->stopPhoneRing();
		}else{
			menu->startPhoneRing();
		}
	}, LV_EVENT_CLICKED, this);

	lv_obj_add_event_cb(*items[0], [](lv_event_t* evt){
		auto menu = static_cast<MainMenu*>(evt->user_data);
		if(menu->findPhoneRinging){
			menu->stopPhoneRing();
		}
	}, LV_EVENT_DEFOCUSED, this);

	statusBar = new StatusBar(*this);
	lv_obj_add_flag(*statusBar, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_pos(*statusBar, 0, 0);

	// Scrolling
	lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_ONE);
	lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_START);
	lv_group_set_wrap(inputGroup, false);

	Events::listen(Facility::Input, &queue);
	Events::listen(Facility::Phone, &queue);
}

MainMenu::~MainMenu(){
	Events::unlisten(&queue);
	lv_img_cache_set_size(LV_IMG_CACHE_DEF_SIZE);
}

void MainMenu::setupItemPaths(Theme theme){
	ItemInfos[0].iconPath = THEMED_FILE(Menu, Find, theme);
	ItemInfos[0].iconAltPath = THEMED_FILE(Menu, Find, theme);
	ItemInfos[1].iconPath = THEMED_FILE(Menu, Lunar, theme);
	ItemInfos[2].iconPath = THEMED_FILE(Menu, Level, theme);
	ItemInfos[3].iconPath = THEMED_FILE(Menu, Theremin, theme);
	ItemInfos[4].iconPath = THEMED_FILE(Menu, Connection, theme);
	ItemInfos[4].iconAltPath = THEMED_FILE(Menu, Connection, theme);
	ItemInfos[5].iconPath = THEMED_FILE(Menu, Rover, theme);
	ItemInfos[6].iconPath = THEMED_FILE(Menu, Settings, theme);
}

void MainMenu::resetMenuIndex(){
	lastIndex = UINT8_MAX;
}

void MainMenu::onStart(){
	lv_indev_wait_release(InputLVGL::getInstance()->getIndev());

	const auto status = (StatusCenter*) Services.get(Service::Status);
	status->blinkAll();
}

void MainMenu::onStarting(){
	// TODO: place all ring phone stuff into setRingAlts

	if(lastIndex == UINT8_MAX){
		lastIndex = 0;
	}

	if(phone.getPhoneType() != Phone::PhoneType::Android){
		lv_obj_add_flag(*items[0], LV_OBJ_FLAG_HIDDEN);

		if(lastIndex == 0){
			lastIndex = 1;
		}
	}

	lv_group_focus_obj(*items[lastIndex]);
	lv_obj_scroll_to_view(*items[lastIndex], LV_ANIM_OFF);

	lastIndex = UINT8_MAX;

	setConnAlts();
}

void MainMenu::onStop(){
	findPhoneRinging = false;
	phone.findPhoneStop();
}

void MainMenu::loop(){
	statusBar->loop();

	handleRing();

	Event evt;
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto data = (Input::Data*) evt.data;
			handleInput(*data);
		}else if(evt.facility == Facility::Phone){
			auto data = (Phone::Event*) evt.data;
			handlePhoneChange(*data);
		}
		free(evt.data);
	}
}

void MainMenu::onClick(){
	auto focused = lv_group_get_focused(inputGroup);
	auto index = lv_obj_get_index(focused);
	if(index >= ItemCount) return;

	lastIndex = index;

	std::function<void()> launcher[] = {
			[](){ },
			[this](){ transition([](){ return std::make_unique<LunarLander>(); }); },
			[this](){ transition([](){ return std::make_unique<Level>(); }); },
			[this](){ transition([](){ return std::make_unique<Theremin>(); }); },
			[](){ },
			[this](){ transition([](){ return std::make_unique<PerseCtrlScreen>(); }); },
			[this](){ transition([](){ return std::make_unique<SettingsScreen>(); }); }
	};

	launcher[index]();
}

void MainMenu::handlePhoneChange(Phone::Event& event){
	auto focused = lv_group_get_focused(inputGroup);
	auto index = lv_obj_get_index(focused);
	auto& findPhone = *items[0];
	bool hiddenBefore = lv_obj_has_flag(findPhone, LV_OBJ_FLAG_HIDDEN);

	if(event.action == Phone::Event::Connected && event.data.phoneType == Phone::PhoneType::Android){
		lv_obj_clear_flag(findPhone, LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_add_flag(findPhone, LV_OBJ_FLAG_HIDDEN);
	}

	if(hiddenBefore != lv_obj_has_flag(findPhone, LV_OBJ_FLAG_HIDDEN)){
		if(!hiddenBefore && index == 0){
			lv_obj_scroll_to_view(*items[1], LV_ANIM_OFF);
			lv_group_focus_obj(*items[1]);
		}else{
			lv_obj_scroll_to_view(*items[index], LV_ANIM_OFF);
			lv_group_focus_obj(*items[index]);
		}
	}

	setConnAlts();
}

void MainMenu::handleInput(Input::Data& event){
	if(event.btn == Input::Alt && event.action == Input::Data::Press){
		transition([](){ return std::make_unique<LockScreen>(); });
	}
}

void MainMenu::setConnAlts(){
	auto connEl = (MenuItemAlt*) items[4];

	const auto connAlt = phone.getPhoneType() == Phone::PhoneType::None
						 ? ItemInfos[4].iconAltPath : ItemInfos[4].iconPath;
	connEl->setAltParams(connAlt, ConnDesc[(int) phone.getPhoneType()]);
}

void MainMenu::startPhoneRing(){
	if(findPhoneRinging) return;

	findPhoneRinging = true;
	phone.findPhoneStart();
	findPhoneCounter = millis();
	findPhoneState = true;
}

void MainMenu::stopPhoneRing(){
	if(!findPhoneRinging) return;

	findPhoneRinging = false;
	phone.findPhoneStop();
}

void MainMenu::handleRing(){
	if(!findPhoneRinging) return;

	if(millis() - findPhoneCounter >= FindPhonePeriod){
		findPhoneCounter = millis();
		findPhoneState = !findPhoneState;
		findPhoneState ? phone.findPhoneStart() : phone.findPhoneStop();
	}
}
