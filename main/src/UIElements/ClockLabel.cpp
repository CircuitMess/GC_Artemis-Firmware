#include "ClockLabel.h"
#include "Util/Services.h"
#include "Util/stdafx.h"
#include "Theme/theme.h"
#include "Settings/Settings.h"

ClockLabel::ClockLabel(lv_obj_t* parent) : LVObject(parent), ts(*((Time*) Services.get(Service::Time))), queue(2){
	lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

	Events::listen(Facility::Time, &queue);
	auto& setts = *(Settings*) Services.get(Service::Settings);
	set24hFormat(setts.get().timeFormat24h);
}

ClockLabel::~ClockLabel(){
	Events::unlisten(&queue);
}

void ClockLabel::loop(){
	if(millis() - lastTimeUpdate > TimeUpdateInterval){
		updateTime(ts.getTime());
	}

	Event evt;
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Time){
			auto data = (Time::Event*) evt.data;
			if(data->action == Time::Event::Updated){
				updateTime(data->updated.time);
			}
		}
		free(evt.data);
	}
}

void ClockLabel::set24hFormat(bool format){
	format24h = format;
}

void ClockLabel::updateTime(const tm& time){
	lastTimeUpdate = millis();

	char clockText[128];
	const char* ampm = nullptr;
	if(format24h){
		snprintf(clockText, sizeof(clockText), "%02d%c%02d", time.tm_hour, time.tm_sec % 2 ? ':' : ' ', time.tm_min);
	}else{
		ampm = time.tm_hour < 12 ? "AM" : "PM";
		int hh = time.tm_hour % 12;
		if(hh == 0){
			hh = 12;
		}
		snprintf(clockText, sizeof(clockText), "%02d%c%02d", hh, time.tm_sec % 2 ? ':' : ' ', time.tm_min);
	}

	updateUI(clockText, ampm);
	lv_obj_refr_size(*this);
	lv_obj_invalidate(*this);
}
