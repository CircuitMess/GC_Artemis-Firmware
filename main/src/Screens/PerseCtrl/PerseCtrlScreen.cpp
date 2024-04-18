#include "PerseCtrlScreen.h"
#include "Services/SleepMan.h"
#include "Devices/Input.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Devices/IMU.h"
#include "Util/Services.h"
#include "vec2.hpp"
#include "geometric.hpp"
#include "trigonometric.hpp"
#include "gtx/vector_angle.hpp"
#include "LV_Interface/FSLVGL.h"

lv_color_t PerseCtrlScreen::Color = lv_color_make(255, 101, 0);

PerseCtrlScreen::PerseCtrlScreen() : wifi(*(WiFiSTA*) Services.get(Service::WiFi)), evts(6){
	wifi.start();

	tcp = std::make_unique<TCPClient>();
	comm = std::make_unique<Comm>(*tcp);

	FSLVGL::unloadCache();

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	feedBuf = (uint8_t*) heap_caps_malloc(160*120*2, MALLOC_CAP_SPIRAM);
	memset(feedBuf, 0, 160*120*2);
	imgDsc.data_size = 160*120*2;
	imgDsc.data = feedBuf;

	feedImg = lv_img_create(*this);
	lv_obj_set_pos(feedImg, 0, 0);
	lv_img_set_src(feedImg, &imgDsc);

	pairLabel = lv_label_create(*this);
	lv_obj_set_size(pairLabel, 128, 20);
	lv_obj_set_style_text_color(pairLabel, Color, 0);
	lv_obj_set_style_text_align(pairLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(pairLabel, "Hold select to pair");
	lv_obj_set_pos(pairLabel, 0, 60);

	Events::listen(Facility::Input, &evts);
	Events::listen(Facility::TCP, &evts);

	heapRep();
}

PerseCtrlScreen::~PerseCtrlScreen(){
	if(auto settings = (Settings*) Services.get(Service::Settings)){
		if(!cacheReturned){
			FSLVGL::loadCache(settings->get().themeData.theme);
		}
	}

	pair.reset();

	if(tcp){
		tcp->disconnect();
	}

	wifi.stop();
	free(feedBuf);
	Events::unlisten(&evts);
}

void PerseCtrlScreen::onStart(){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(false);
}

void PerseCtrlScreen::onStop(){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(true);
}

void PerseCtrlScreen::loop(){
	for(Event evt{}; evts.get(evt, 0); ){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Select && eventData->action == Input::Data::Press){
				pair = std::make_unique<PairService>(wifi, *tcp);
			}else if(eventData->btn == Input::Select && eventData->action == Input::Data::Release){
				if(pair){
					lv_label_set_text(pairLabel, "Hold select to pair");
				}

				pair.reset();
			}else if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				free(evt.data);

				lv_obj_clean(*this);

				lv_obj_t* loading = lv_label_create(*this);
				lv_obj_set_size(loading, 128, 12);
				lv_obj_set_style_text_color(loading, Color, 0);
				lv_obj_set_style_text_align(loading, LV_TEXT_ALIGN_CENTER, 0);
				lv_obj_center(loading);
				lv_label_set_text(loading, "Loading...");

				lv_obj_invalidate(*this);
				vTaskDelay(LV_DISP_DEF_REFR_PERIOD);
				lv_timer_handler();

				auto settings = (Settings*) Services.get(Service::Settings);
				if(settings){
					cacheReturned = true;
					FSLVGL::loadCache(settings->get().themeData.theme);
				}

				transition([](){ return std::make_unique<MainMenu>(); });
				return;
			}else if((eventData->btn == Input::Up || eventData->btn == Input::Down) && eventData->action == Input::Data::Release){
				camDir = 0;
			}else if(eventData->btn == Input::Up && eventData->action == Input::Data::Press){
				camDir = 5;
			}else if(eventData->btn == Input::Down && eventData->action == Input::Data::Press){
				camDir = -5;
			}
		}
		else if(evt.facility == Facility::TCP){
			auto eventData = (TCPClient::Event*) evt.data;

			if(eventData->status == TCPClient::Event::Status::Disconnected){
				paired = false;
				memset(feedBuf, 0, 160*120*2);
				lv_obj_invalidate(feedImg);
				lv_label_set_text(pairLabel, "Disconnected\nHold select to pair");
			}
		}
		free(evt.data);
	}

	if(pair){
		if(pair->getState() == PairService::State::Pairing){
			lv_label_set_text(pairLabel, "Pairing...");
		}else{
			if(pair->getState() == PairService::State::Fail){
				paired = false;
				lv_label_set_text(pairLabel, "Pairing failed\nHold select to pair");
			}else{
				lv_label_set_text(pairLabel, "");
				paired = true;

				if(comm){
					comm->sendFeedQuality(30);
				}
			}

			pair.reset();
		}
	}

	if(!paired) return;

	feed.nextFrame([this](const DriveInfo& info, const ::Color* buf){
		memcpy(feedBuf, buf, 160*120*2);
		lv_obj_invalidate(feedImg);
	});

	if(sendTime == 0){
		sendTime = millis();
		return;
	}

	auto now = millis();
	if(now - sendTime < SendInterval) return;
	sendTime = now;

	if(camDir != 0){
		camPos = std::clamp(camPos + camDir, 0, 100);

		if(comm){
			comm->sendCameraRotation(camPos);
		}
	}

	auto imu = (IMU*) Services.get(Service::IMU);
	auto sample = imu->getSample();
	glm::vec2 dir = {
			std::clamp(sample.accelY, -1.0, 1.0),
			std::clamp(-sample.accelX, -1.0, 1.0)
	};

	const auto len = std::clamp(glm::length(dir), 0.0f, 1.0f);

	if(len < 0.2){
		if(comm){
			comm->sendDriveDir({ 0, 0.0f });
		}

		return;
	}

	auto angle = glm::degrees(glm::angle(glm::normalize(dir), { 0.0, 1.0 }));
	if(dir.x < 0){
		angle = 360.0f - angle;
	}

	static constexpr float circParts = 360.0/8.0;

	float calcAngle = angle + circParts/2.0;
	if(calcAngle >= 360){
		calcAngle -= 360.0f;
	}
	const uint8_t number = std::floor(calcAngle / circParts);

	if(comm){
		comm->sendDriveDir({ number, len });
	}
}
