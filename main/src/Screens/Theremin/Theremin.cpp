#include "Theremin.h"
#include <algorithm>
#include "Util/Services.h"
#include "Util/stdafx.h"
#include "Devices/Input.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/StatusCenter.h"
#include "Services/SleepMan.h"


Theremin::Theremin() : audio(*(ChirpSystem*) Services.get(Service::Audio)), sem(xSemaphoreCreateBinary()),
					   timer(getToneDuration(sequence.getSize()), timerCB, sem),
					   audioThread([this](){ audioThreadFunc(); }, "Theremin audio", 2048, 5, 1), imu((IMU*) Services.get(Service::IMU)),
					   pitchFilter(filterStrength), rollFilter(filterStrength), queue(4){
	buildUI();
	sequence.refresh();

	xSemaphoreGive(sem);
}


Theremin::~Theremin(){
	vSemaphoreDelete(sem);
}

void Theremin::setOrientation(float pitch, float roll){
	roll = -roll;
	pitch = -pitch;

	pitch = std::clamp(pitch, -AngleConstraint, AngleConstraint);
	roll = std::clamp(roll, -AngleConstraint, AngleConstraint);

	const auto horizontalX = (int16_t) (((roll + AngleConstraint) / (2 * AngleConstraint)) * SliderRange);
	const auto verticalY = (int16_t) (((pitch + AngleConstraint) / (2 * AngleConstraint)) * SliderRange);

	lv_slider_set_value(sliderVertical, verticalY, LV_ANIM_OFF);
	lv_slider_set_value(sliderHorizontal, horizontalX, LV_ANIM_OFF);

	const auto mappedNote = (uint16_t) map(roll, -AngleConstraint, AngleConstraint, 0, ArpeggioSequence::MaxBaseNoteIndex);
	const auto mappedSize = (uint8_t) map(pitch, -AngleConstraint, AngleConstraint, 1, ArpeggioSequence::MaxSequenceSize);

	if(mappedNote != sequence.getBaseNoteIndex()){
		sequence.setBaseNoteIndex(mappedNote);
	}

	if(mappedSize != sequence.getSize()){
		sequence.setSize(mappedSize);
		timer.stop();
		timer.setPeriod(getToneDuration(sequence.getSize()));
		timer.start();
	}
}

void Theremin::onStart(){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(false);

	auto status = (StatusCenter*) Services.get(Service::Status);
	status->blockAudio(true);

	audio.setPersistentAttach(true);

	const IMU::Sample reading = imu->getSample();
	const PitchRoll pitchRoll = { -(float) reading.accelY, -(float) reading.accelX };
	pitchFilter.reset(pitchRoll.pitch);
	rollFilter.reset(pitchRoll.roll);
	setOrientation(pitchRoll.pitch, pitchRoll.roll);

	abortFlag = false;
	audioThread.start();
	timer.start();
	Events::listen(Facility::Input, &queue);
}

void Theremin::onStop(){
	timer.stop();
	audio.stop();
	audio.setPersistentAttach(false);

	audioThread.stop(0);
	abortFlag = true;
	xSemaphoreGive(sem);
	while(audioThread.running()){
		vTaskDelay(1);
	}

	Events::unlisten(&queue);

	auto status = (StatusCenter*) Services.get(Service::Status);
	status->blockAudio(false);

	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	sleep->enAutoSleep(true);
}

void Theremin::loop(){

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

	const IMU::Sample reading = imu->getSample();
	const PitchRoll pitchRoll = { (float) pitchFilter.update(-reading.accelY), (float) rollFilter.update(-reading.accelX) };
	setOrientation(pitchRoll.pitch, pitchRoll.roll);
}

void IRAM_ATTR Theremin::timerCB(void* arg){
	BaseType_t priority = pdFALSE;
	xSemaphoreGiveFromISR(arg, &priority);
}

void Theremin::audioThreadFunc(){
	while(!xSemaphoreTake(sem, portMAX_DELAY));
	timer.stop();

	if(abortFlag) return;

	if(sequenceIndex >= sequence.getSize()){
		sequenceIndex = 0;
	}

	if(sequenceIndex == 0){
		sequence.refresh();
		timer.setPeriod(getToneDuration(sequence.getSize()));
	}

	const auto freq = sequence.getTones()[sequenceIndex];
	const uint16_t toneDuration = getToneDuration(sequence.getSize()) / 2;
	const Sound sound = {{ freq, freq, toneDuration },
						 { 0,    0,    toneDuration }};

	audio.play(sound);
	timer.start();

	sequenceIndex = sequenceIndex + 1;
}

constexpr uint32_t Theremin::getToneDuration(uint8_t sequenceSize){
	return SequenceDuration / sequenceSize;
}

void Theremin::buildUI(){
	bg = lv_obj_create(*this);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_style_bg_img_src(bg, "S:/theremin/bg.bin", 0);
	lv_obj_set_style_bg_img_tiled(bg, true, 0);


	sliderHorizontal = lv_slider_create(bg);
	lv_obj_set_pos(sliderHorizontal, HorizontalBarX, HorizontalBarY);
	lv_obj_set_style_opa(sliderHorizontal, LV_OPA_TRANSP, LV_PART_MAIN | LV_PART_INDICATOR);
	lv_obj_set_size(sliderHorizontal, SliderLength, SliderWidth);
	lv_obj_set_style_bg_img_src(sliderHorizontal, "S:/theremin/dotHorizontal.bin", LV_PART_KNOB);
	lv_obj_set_style_pad_hor(sliderHorizontal, 2, LV_PART_MAIN);
	lv_slider_set_range(sliderHorizontal, 0, SliderRange);


	sliderVertical = lv_slider_create(bg);
	lv_obj_set_pos(sliderVertical, VerticalBarX, VerticalBarY);
	lv_obj_set_style_opa(sliderVertical, LV_OPA_TRANSP, LV_PART_MAIN | LV_PART_INDICATOR);
	lv_obj_set_size(sliderVertical, SliderWidth, SliderLength);
	lv_obj_set_style_bg_img_src(sliderVertical, "S:/theremin/dotVertical.bin", LV_PART_KNOB);
	lv_obj_set_style_pad_ver(sliderVertical, 2, LV_PART_MAIN);
	lv_slider_set_range(sliderVertical, 0, SliderRange);
}
