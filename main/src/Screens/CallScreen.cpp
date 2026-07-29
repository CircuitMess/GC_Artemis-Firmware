#include "CallScreen.h"
#include "Settings/Settings.h"
#include "Theme/theme.h"
#include "Util/Services.h"
#include "Notifs/Phone.h"
#include "Screens/Lock/LockScreen.h"
#include "Services/StatusCenter.h"

CallScreen::CallScreen() : evts(6){
	auto phone = (Phone* ) Services.get(Service::Phone);

	notif = phone->getCall();
	if(notif.category != Notif::Category::IncomingCall){
		lv_async_call([](void* data){
			auto scr = (CallScreen*) data;
			scr->transition([](){ return std::make_unique<LockScreen>(); });
		}, this);
		return;
	}

	buildUI();
	Events::listen(Facility::Phone, &evts);
	Events::listen(Facility::Input, &evts);
}

CallScreen::~CallScreen(){
	Events::unlisten(&evts);
}

void CallScreen::onIgnore(){
	auto phone = (Phone* ) Services.get(Service::Phone);
	phone->callIgnore(notif.uid);
	transition([](){ return std::make_unique<LockScreen>(); });
}

void CallScreen::onReject(){
	auto phone = (Phone* ) Services.get(Service::Phone);
	phone->callReject(notif.uid);
	transition([](){ return std::make_unique<LockScreen>(); });
}

void CallScreen::loop(){
	Event evt;
	if(evts.get(evt, 0)){
		if(evt.facility == Facility::Phone){
			auto data = (Phone::Event*) evt.data;
			if(data->action == Phone::Event::Removed && data->data.addChgRem.id == notif.uid){
				free(evt.data);
				transition([](){ return std::make_unique<LockScreen>(); });
				return;
			}
		}else if(evt.facility == Facility::Input){
			auto data = (Input::Data*) evt.data;
			if(data->btn == Input::Alt && data->action == Input::Data::Press){
				free(evt.data);
				onIgnore();
				return;
			}
		}

		free(evt.data);
	}

	if(millis() - notifTime < NotifInterval) return;
	notifTime = millis();

	auto status = (StatusCenter*) Services.get(Service::Status);
	status->blinkAllTwice();
}

void CallScreen::onStart(){
	notifTime = millis();
}

void CallScreen::buildUI(){
	auto settings = (Settings*) Services.get(Service::Settings);

	lv_style_set_text_color(textStyle, settings->get().themeData.primaryColor);
	lv_style_set_text_align(textStyle, LV_TEXT_ALIGN_CENTER);
	lv_style_set_width(textStyle, lv_pct(100));
	lv_style_set_text_font(textStyle, &landerfont);

	lv_obj_set_size(*this, 128, 128);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_set_style_border_width(*this, 1, 0);
	lv_obj_set_style_border_opa(*this, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(*this, settings->get().themeData.primaryColor, 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(*this, settings->get().themeData.backgroundColor, 0);

	lv_obj_set_style_pad_all(*this, 0, 0);
	lv_obj_set_style_pad_gap(*this, 5, 0);

	auto labelTitle = lv_label_create(*this);
	lv_label_set_text(labelTitle, "Incoming\nCall");
	lv_obj_add_style(labelTitle, textStyle, 0);
	lv_obj_set_style_text_letter_space(labelTitle, -1, 0);
	lv_obj_set_style_text_font(labelTitle, &lv_font_unscii_16, 0);
	lv_obj_set_style_pad_bottom(labelTitle, 5, 0);

	auto labelName = lv_label_create(*this);
	lv_label_set_text(labelName, notif.title.c_str());
	lv_label_set_long_mode(labelName, LV_LABEL_LONG_SCROLL);
	lv_obj_add_style(labelName, textStyle, 0);
	lv_obj_set_style_text_font(labelName, &lv_font_unscii_8, 0);

	auto labelNumber = lv_label_create(*this);
	lv_label_set_text(labelNumber, notif.message.c_str());
	lv_label_set_long_mode(labelNumber, LV_LABEL_LONG_SCROLL);
	lv_obj_add_style(labelNumber, textStyle, 0);
	lv_obj_set_style_text_font(labelNumber, &devin, 0);

	auto btns = lv_obj_create(*this);
	lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
	lv_obj_set_size(btns, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_gap(btns, 5, 0);
	lv_obj_set_style_pad_top(btns, 5, 0);

	lv_style_set_pad_hor(btnStyle, 4);
	lv_style_set_pad_ver(btnStyle, 3);
	lv_style_set_text_color(btnStyle, settings->get().themeData.primaryColor);
	lv_style_set_text_font(btnStyle, &landerfont);

	lv_style_set_border_width(btnStyleFocus, 1);
	lv_style_set_border_opa(btnStyleFocus, LV_OPA_COVER);
	lv_style_set_border_color(btnStyleFocus, settings->get().themeData.primaryColor);
	lv_style_set_pad_hor(btnStyleFocus, 3);
	lv_style_set_pad_ver(btnStyleFocus, 2);

	lv_style_set_bg_color(btnStylePress, settings->get().themeData.primaryColor);
	lv_style_set_bg_opa(btnStylePress, LV_OPA_COVER);

	auto btnIgnore = lv_obj_create(btns);
	lv_obj_clear_flag(btnIgnore, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(btnIgnore, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_set_size(btnIgnore, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_add_style(btnIgnore, btnStyle, 0);
	lv_obj_add_style(btnIgnore, btnStyleFocus, LV_STATE_FOCUSED);
	lv_obj_add_style(btnIgnore, btnStylePress, LV_STATE_PRESSED);
	lv_group_add_obj(inputGroup, btnIgnore);
	auto labelIgnore = lv_label_create(btnIgnore);
	lv_label_set_text(labelIgnore, "Ignore");

	lv_obj_add_event_cb(btnIgnore, [](lv_event_t* e){
		auto scr = (CallScreen*) e->user_data;
		scr->onIgnore();
	}, LV_EVENT_CLICKED, this);

	auto btnReject = lv_obj_create(btns);
	lv_obj_clear_flag(btnReject, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(btnReject, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_set_size(btnReject, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_add_style(btnReject, btnStyle, 0);
	lv_obj_add_style(btnReject, btnStyleFocus, LV_STATE_FOCUSED);
	lv_obj_add_style(btnReject, btnStylePress, LV_STATE_PRESSED);
	lv_group_add_obj(inputGroup, btnReject);
	auto labelReject = lv_label_create(btnReject);
	lv_label_set_text(labelReject, "Reject");

	lv_obj_add_event_cb(btnReject, [](lv_event_t* e){
		auto scr = (CallScreen*) e->user_data;
		scr->onReject();
	}, LV_EVENT_CLICKED, this);

	lv_group_set_wrap(inputGroup, false);
}
