#include "PerseCtrlScreen.h"
#include "Devices/Input.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Devices/IMU.h"
#include "Util/Services.h"
#include "vec2.hpp"
#include "geometric.hpp"
#include "trigonometric.hpp"
#include "gtx/vector_angle.hpp"

PerseCtrlScreen::PerseCtrlScreen() : comm(tcp), evts(6){
	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);

	pair = std::make_unique<PairService>(wifi, tcp);

	feedBuf = (uint8_t*) heap_caps_malloc(160*120*2, MALLOC_CAP_SPIRAM);
	memset(feedBuf, 0, 160*120*2);
	imgDsc.data_size = 160*120*2;
	imgDsc.data = feedBuf;

	feedImg = lv_img_create(*this);
	lv_obj_set_pos(feedImg, 0, 0);
	lv_img_set_src(feedImg, &imgDsc);

	pairLabel = lv_label_create(*this);
	lv_obj_set_size(pairLabel, 128, 12);
	lv_obj_set_style_text_color(pairLabel, lv_color_make(0, 220, 0), 0);
	lv_obj_set_style_text_align(pairLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(pairLabel, "Pairing...");
	lv_obj_set_pos(pairLabel, 0, 60);

	Events::listen(Facility::Input, &evts);

	heapRep();
}

PerseCtrlScreen::~PerseCtrlScreen(){
	pair.reset();
	tcp.disconnect();
	free(feedBuf);
	Events::unlisten(&evts);
}

void PerseCtrlScreen::loop(){
	Event evt{};
	if(evts.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				free(evt.data);
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
		free(evt.data);
	}

	if(pair && pair->getState() != PairService::State::Pairing){
		if(pair->getState() == PairService::State::Fail){
			lv_label_set_text(pairLabel, "Pairing failed");
			printf("Pair failed\n");
		}else{
			printf("Pair success\n");
			lv_obj_del(pairLabel);
			paired = true;
			comm.sendFeedQuality(1);
		}

		printf("Deleting pair...\n");
		pair.reset();
		printf("Done\n");
	}

	if(!paired) return;

	feed.nextFrame([this](const DriveInfo& info, const Color* buf){
		memcpy(feedBuf, buf, 160*120*2);
		lv_obj_invalidate(feedImg);
		// printf("Frame\n");
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
		comm.sendCameraRotation(camPos);
	}

	auto imu = (IMU*) Services.get(Service::IMU);
	auto sample = imu->getSample();
	glm::vec2 dir = {
			std::clamp(sample.accelY, -1.0, 1.0),
			std::clamp(-sample.accelX, -1.0, 1.0)
	};

	const auto len = std::clamp(glm::length(dir), 0.0f, 1.0f);

	// printf("Dir: [%.2f %.2f], len: %.2f\n", dir.x, dir.y, len);

	if(len < 0.2){
		comm.sendDriveDir({ 0, 0.0f });
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
	const uint8_t numer = std::floor(calcAngle / circParts);

	comm.sendDriveDir({ numer, len });
}

