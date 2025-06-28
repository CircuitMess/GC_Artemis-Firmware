#include <esp_log.h>
#include "Time.h"
#include "Util/stdafx.h"
#include "Util/Events.h"

static const char* TAG = "Time";

Time::Time(RTC& rtc) : SleepyThreaded(UpdateInterval, "Time", 2 * 1024, 6, 0), rtc(rtc){
	updateFromRTC();
	start();
}

Time::~Time(){
	stop();
}

std::tm Time::getTime() const{
	time_t currentTime = time + (millis() - updateTime) / 1000;
	tm ret = {};
	gmtime_r(&currentTime, &ret);
	return ret;
}

void Time::setTime(tm time_tm){
	ESP_LOGI(TAG, "Updating time by tm");

	resetTime();
	updateTime = millis();

	rtc.setTime(time_tm);
	Time::time = mktime(&time_tm);

	Events::post(Facility::Time, Time::Event { .action = Event::Updated, .updated = { .time = time_tm } });
}

void Time::setTime(time_t time){
	ESP_LOGI(TAG, "Updating time by time_t");

	resetTime();
	updateTime = millis();

	tm time_tm = {};
	gmtime_r(&time, &time_tm);

	rtc.setTime(time_tm);
	Time::time = time;

	Events::post(Facility::Time, Time::Event { .action = Event::Updated, .updated = { .time = time_tm } });
}

tm Time::updateFromRTC(){
	resetTime();
	updateTime = millis();

	tm time_tm = rtc.getTime();
	time = mktime(&time_tm);
	return time_tm;
}

void Time::sleepyLoop(){
	ESP_LOGI(TAG, "Scheduled RTC sync");
	auto time_tm = updateFromRTC();
	Events::post(Facility::Time, Time::Event { .action = Event::Updated, .updated = { .time = time_tm } });
}
