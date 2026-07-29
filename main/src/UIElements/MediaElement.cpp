#include "MediaElement.h"
#include "LV_Interface/FSLVGL.h"
#include "Theme/theme.h"
#include "Settings/Settings.h"
#include "Util/Services.h"
#include "Util/stdafx.h"
#include <utility>

static constexpr const char* Plays[] = {
		"/media/playing/0.bin",
		"/media/playing/1.bin",
		"/media/playing/2.bin",
		"/media/playing/3.bin",
		"/media/playing/4.bin",
		"/media/playing/5.bin",
		"/media/playing/6.bin",
		"/media/playing/7.bin",
		"/media/play.bin",
		"/media/play-act.bin",
		"/media/pause.bin",
		"/media/pause-act.bin",
		"/media/next.bin",
		"/media/next-act.bin",
		"/media/prev.bin",
		"/media/prev-act.bin",
};

MediaElement::MediaElement(lv_obj_t* parent) : LVSelectable(parent), playing(*this, "S:/media/playing"){
	buildUI();

	lv_obj_add_event_cb(*this, [](lv_event_t* e){
		auto el = (MediaElement*) e->user_data;
		el->onSelect();
	}, LV_EVENT_CLICKED, this);

	lv_obj_add_event_cb(*this, [](lv_event_t* e){
		auto el = (MediaElement*) e->user_data;
		el->onDeselect();
	}, LV_EVENT_READY, this);

	lv_group_set_edge_cb(inputGroup, [](lv_group_t* grp, bool side){
		auto el = (MediaElement*) grp->user_data;
		if(side){
			el->next();
		}else{
			el->prev();
		}
	});

	inputGroup->user_data = this;
	phone = (Phone*) Services.get(Service::Phone);

	for(const auto file : Plays){
		FSLVGL::addToCache(file);
	}
}

MediaElement::~MediaElement(){
	for(const auto file : Plays){
		FSLVGL::removeFromCache(file);
	}
}

void MediaElement::loop(){
	if(timerTime == 0 || millis() - timerTime <= TimerPeriod) return;
	timerCb();
	timerTime = 0;
}

void MediaElement::timer(std::function<void()> cb){
	timerCb = std::move(cb);
	timerTime = millis();
}

void MediaElement::setInfo(const MediaInfo& info){
	lv_label_set_text(title, info.title.c_str());
	lv_label_set_text(artist, info.artist.c_str());
	lv_label_set_text(album, info.album.c_str());
}

void MediaElement::setState(MediaState state){
	auto* img = lv_obj_get_child(btnPlay, 0);
	if(state == MediaState::Playing){
		lv_img_set_src(img, "S:/media/pause.bin");
		playing.start();
		lv_obj_clear_flag(playing, LV_OBJ_FLAG_HIDDEN);
	}else if(state == MediaState::Paused){
		lv_img_set_src(img, "S:/media/play.bin");
		playing.stop();
		lv_obj_add_flag(playing, LV_OBJ_FLAG_HIDDEN);
	}else if(state == MediaState::Stopped){
		playing.stop();
	}
	this->state = state;
	actTime = millis();
}

void MediaElement::onSelect(){
	lv_obj_set_style_bg_opa(btns, LV_OPA_30, 0);
}

void MediaElement::onDeselect(){
	lv_obj_set_style_bg_opa(btns, LV_OPA_0, 0);
}

void MediaElement::playPause(){
	if(millis() - actTime <= ActCooldown) return;
	actTime = millis();

	auto* img = lv_obj_get_child(btnPlay, 0);

	if(state == MediaState::Playing){
		state = MediaState::Paused;

		phone->doMediaPause();

		playing.stop();
		lv_obj_add_flag(playing, LV_OBJ_FLAG_HIDDEN);

		lv_img_set_src(img, "S:/media/play-act.bin");
		timer([img](){ lv_img_set_src(img, "S:/media/play.bin"); });
	}else if(state == MediaState::Paused){
		state = MediaState::Playing;

		phone->doMediaPlay();

		playing.start();
		lv_obj_clear_flag(playing, LV_OBJ_FLAG_HIDDEN);

		lv_img_set_src(img, "S:/media/pause-act.bin");
		timer([img](){ lv_img_set_src(img, "S:/media/pause.bin"); });
	}
}

void MediaElement::prev(){
	if(millis() - actTime <= ActCooldown) return;
	actTime = millis();

	phone->doMediaPrev();

	auto* img = lv_obj_get_child(btnPrev, 0);
	lv_img_set_src(img, "S:/media/prev-act.bin");

	timer([img](){ lv_img_set_src(img, "S:/media/prev.bin"); });
}

void MediaElement::next(){
	if(millis() - actTime <= ActCooldown) return;
	actTime = millis();

	phone->doMediaNext();

	auto* img = lv_obj_get_child(btnNext, 0);
	lv_img_set_src(img, "S:/media/next-act.bin");

	timer([img](){ lv_img_set_src(img, "S:/media/next.bin"); });
}

void MediaElement::buildUI(){
	auto settings = (Settings*) Services.get(Service::Settings);

	lv_style_set_text_color(textStyle, settings->get().themeData.primaryColor);
	lv_style_set_text_align(textStyle, LV_TEXT_ALIGN_CENTER);
	lv_style_set_text_font(textStyle, &devin);
	lv_style_set_width(textStyle, lv_pct(100));

	lv_obj_set_size(*this, 128, 128);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_set_style_border_width(*this, 1, 0);
	lv_obj_set_style_border_opa(*this, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(*this, settings->get().themeData.primaryColor, 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(*this, settings->get().themeData.backgroundColor, 0);

	lv_obj_set_style_pad_all(*this, 0, 0);
	lv_obj_set_style_pad_gap(*this, 10, 0);

	title = lv_label_create(*this);
	artist = lv_label_create(*this);
	album = lv_label_create(*this);

	for(auto* label : { title, artist, album }){
		lv_obj_add_style(label, textStyle, 0);
		lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	}

	lv_obj_set_style_pad_bottom(album, 3, 0);

	btns = lv_obj_create(*this);
	lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btns, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_size(btns, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_gap(btns, 7, 0);
	lv_obj_set_style_pad_hor(btns, 2, 0);
	lv_obj_set_style_pad_ver(btns, 3, 0);
	lv_obj_set_style_radius(btns, 2, 0);
	lv_obj_set_style_bg_color(btns, lv_color_white(), 0);

	btnPrev = lv_obj_create(btns);
	auto* img = lv_img_create(btnPrev);
	lv_img_set_src(img, "S:/media/prev.bin");

	btnPlay = lv_obj_create(btns);
	img = lv_img_create(btnPlay);
	lv_img_set_src(img, "S:/media/pause.bin");

	btnNext = lv_obj_create(btns);
	img = lv_img_create(btnNext);
	lv_img_set_src(img, "S:/media/next.bin");

	for(auto* btn : { btnPrev, btnPlay, btnNext }){
		lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	}

	lv_obj_add_flag(playing, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_set_pos(playing, 0, 94);
	playing.start();
}


