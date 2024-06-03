#include "Level.h"
#include <algorithm>
#include <vec2.hpp>
#include <geometric.hpp>
#include <esp_log.h>
#include "../Util/Services.h"
#include "Devices/Input.h"
#include "Screens/MainMenu/MainMenu.h"
#include "LV_Interface/FSLVGL.h"
#include "Services/SleepMan.h"
#include "Filepaths.hpp"

Level::Level() : imu((IMU*) Services.get(Service::IMU)), reader([this](){ readerFunc(); }, "reader", 2048, 5, 1), data(QueueSize),
				 pitchFilter(filterStrength), rollFilter(filterStrength), queue(4){
	FSLVGL::addToCache("/level/bg.bin");

	bg = lv_obj_create(*this);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_style_bg_img_src(bg, File::Level::Background, LV_STATE_DEFAULT);

	bubbleCenter = lv_img_create(bg);
	lv_img_set_src(bubbleCenter, File::Level::Bubble);
	bubbleHorizontal = lv_img_create(bg);
	lv_img_set_src(bubbleHorizontal, File::Level::Bubble);
	lv_obj_set_y(bubbleHorizontal, 100);
	bubbleVertical = lv_img_create(bg);
	lv_img_set_src(bubbleVertical, File::Level::Bubble);
	lv_obj_set_x(bubbleVertical, 100);


	markingsCenter = lv_img_create(bg);
	lv_img_set_src(markingsCenter, File::Level::MarkingsCenter);
	lv_obj_set_pos(markingsCenter, 7, 7);
	markingsHorizontal = lv_img_create(bg);
	lv_img_set_src(markingsHorizontal, File::Level::MarkingsHorizontal);
	lv_obj_set_pos(markingsHorizontal, 32, 96);
	markingsVertical = lv_img_create(bg);
	lv_img_set_src(markingsVertical, File::Level::MarkingsVertical);
	lv_obj_set_pos(markingsVertical, 96, 31);
}

Level::~Level(){
	FSLVGL::removeFromCache("/level/bg.bin");
}

void Level::setOrientation(double pitch, double roll){
	pitch = -pitch;
	pitch = std::clamp(pitch, -AngleConstraint, AngleConstraint);
	roll = std::clamp(roll, -AngleConstraint, AngleConstraint);

	const auto horizontalX = (int16_t) (((roll + AngleConstraint) / (2 * AngleConstraint)) * (HorizontalConstr.max - HorizontalConstr.min) +
										HorizontalConstr.min);
	const auto verticalY = (int16_t) (((pitch + AngleConstraint) / (2 * AngleConstraint)) * (VerticalConstr.max - VerticalConstr.min) + VerticalConstr.min);

	const glm::vec2 vec = { roll / AngleConstraint, pitch / AngleConstraint };
	auto centerVec = vec * (float) CenterConstr;
	if(glm::length(centerVec) > CenterConstr){
		centerVec *= (CenterConstr / glm::length(centerVec));
	}

	lv_obj_set_pos(bubbleCenter, (int16_t) (CenterPos + centerVec.x), (int16_t) (CenterPos + centerVec.y));
	lv_obj_set_x(bubbleHorizontal, horizontalX);
	lv_obj_set_y(bubbleVertical, verticalY);

}

void Level::loop(){
	Event evt{};
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				free(evt.data);
				transition([](){ return std::make_unique<MainMenu>(); });
				return;
			}
		}
		free(evt.data);
	}

	PitchRoll sample{};
	if(!data.get(sample, 0)){
		return;
	}

	setOrientation(sample.pitch, sample.roll);
}

void Level::onStart(){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(false);

	if(imu == nullptr){
		ESP_LOGE("Level", "IMU service error\n");
		return;
	}
	reader.start();
	Events::listen(Facility::Input, &queue);
}

void Level::onStarting(){
	const IMU::Sample reading = imu->getSample();
	PitchRoll pitchRoll = { -reading.accelY, -reading.accelX };

	if(Settings* settings = (Settings*) Services.get(Service::Settings)){
		if(settings->get().screenRotate){
			pitchRoll = { reading.accelY, reading.accelX };
		}
	}

	pitchFilter.reset(pitchRoll.pitch);
	rollFilter.reset(pitchRoll.roll);
	setOrientation(pitchRoll.pitch, pitchRoll.roll);
}

void Level::readerFunc(){
	const IMU::Sample reading = imu->getSample();
	PitchRoll pitchRoll = {};

	if(Settings* settings = (Settings*) Services.get(Service::Settings)){
		if(settings->get().screenRotate){
			pitchRoll = { pitchFilter.update(reading.accelY), rollFilter.update(reading.accelX) };
		}else{
			pitchRoll = { pitchFilter.update(-reading.accelY), rollFilter.update(-reading.accelX) };
		}
	}

	data.post(pitchRoll);

	vTaskDelay(ReaderDelay);
}

void Level::onStop(){
	reader.stop();
	Events::unlisten(&queue);

	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(true);
}
