#include <string>
#include <cstdio>
#include <esp_log.h>
#include "LVGIF.h"

static const char* tag = "LVGIF";

LVGIF::LVGIF(lv_obj_t* parent, const char* path) : LVObject(parent), path(path){
	auto strpath = std::string(path);
	if(strpath.find("S:") == 0){
		strpath = strpath.substr(2);
	}
	auto descPath = "/spiffs" + strpath + "/desc.bin";

	pathLen = descPath.length() + 10;
	imgPath = new char[pathLen];
	auto f = fopen(descPath.c_str(), "r");

	if(f == nullptr){
		ESP_LOGE(tag, "Couldn't open GIF descriptor file at %s", descPath.c_str());
		return;
	}

	//read w and h as uint16_t variables, most significant byte is read first
	fread(((uint8_t*) &w) + 1, 1, 1, f);
	fread(((uint8_t*) &w), 1, 1, f);
	fread(((uint8_t*) &h) + 1, 1, 1, f);
	fread(((uint8_t*) &h), 1, 1, f);
	uint32_t length;
	fread(((uint8_t*) &length) + 3, 1, 1, f);
	fread(((uint8_t*) &length) + 2, 1, 1, f);
	fread(((uint8_t*) &length) + 1, 1, 1, f);
	fread(((uint8_t*) &length), 1, 1, f);
	durations.reserve(length);
	for(int i = 0; i < length; i++){
		uint16_t duration;
		fread(((uint8_t*) &duration) + 1, 1, 1, f);
		fread(((uint8_t*) &duration), 1, 1, f);
		durations.push_back(duration * 10);
	}

	fclose(f);

	lv_obj_set_size(obj, w, h);
	lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
	img = lv_img_create(obj);

	sprintf(imgPath, "%s/%lu.bin", path, index);
	lv_img_set_src(img, imgPath);
	timer = lv_timer_create([](lv_timer_t* t){
		auto gif = (LVGIF*) t->user_data;
		gif->index++;
		if(gif->index >= gif->durations.size()){
			if(gif->cb) gif->cb();
			if(gif->loopType == LoopType::Single){
				return;
			}else if(gif->loopType == LoopType::On){
				gif->index = 0;
			}
		}

		lv_timer_set_period(gif->timer, gif->durations[gif->index]);
		lv_img_cache_invalidate_src(gif->imgPath);
		sprintf(gif->imgPath, "%s/%lu.bin", gif->path, gif->index);
		lv_img_set_src(gif->img, gif->imgPath);

	}, durations[index], this);
	lv_timer_pause(timer);
}

LVGIF::~LVGIF(){
	lv_timer_del(timer);
	delete imgPath;
}

void LVGIF::start(){
	lv_timer_resume(timer);
}

void LVGIF::stop(){
	lv_timer_pause(timer);
}

void LVGIF::reset(){
	index = 0;
	sprintf(imgPath, "%s/%lu.bin", path, index);
	lv_img_set_src(img, imgPath);
	lv_timer_set_period(timer, durations[index]);
	lv_timer_reset(timer);
	lv_timer_resume(timer);
}

void LVGIF::setLooping(LVGIF::LoopType loop){
	loopType = loop;
}

void LVGIF::setDoneCallback(std::function<void()> cb){
	this->cb = cb;
}

void LVGIF::setImage(size_t index){
	this->index = index;
	sprintf(imgPath, "%s/%d.bin", path, index);
	lv_img_set_src(img, imgPath);
	lv_timer_reset(timer);
	lv_timer_resume(timer);
}

size_t LVGIF::getNumFrames() const{
	return durations.size();
}
