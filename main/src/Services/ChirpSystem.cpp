#include <esp_log.h>
#include "ChirpSystem.h"
#include "Util/stdafx.h"

static const char* TAG = "ChirpSystem";

ChirpSystem::ChirpSystem(PWM& pwm) : Threaded("ChirpSystem", 2 * 1024, configMAX_PRIORITIES-1, 0), pwm(pwm), queue(xQueueCreate(QueueLength, sizeof(QueueItem))),
timerSem(xSemaphoreCreateBinary()), timer(MinimumLength * 1000, isr, timerSem), sleepLock(ESP_PM_APB_FREQ_MAX){

	pwm.detach();
	xSemaphoreGive(timerSem);
	start();
}

ChirpSystem::~ChirpSystem(){
	stop();
	Threaded::stop();
	vSemaphoreDelete(timerSem);
	vQueueDelete(queue);
	detach();
}

void ChirpSystem::play(std::initializer_list<Chirp> sound){
	play((Sound) sound);
}

void ChirpSystem::play(const Sound& sound){
	if(mute) return;

	if(!pwmPersistence){
		attach();
	}

	uint32_t totalLength = 0;
	for(const Chirp& chirp : sound){
		totalLength += chirp.duration;
	}
	if(totalLength > MaxLength){
		ESP_LOGE(TAG, "Sound is too long! Max length is %lums, sound is %lums", MaxLength, totalLength);
		return;
	}

	xQueueReset(queue);

	for(const Chirp& chirp : sound){
		if(chirp.endFreq == chirp.startFreq){
			QueueItem item = { QueueItem::Type::Tone, { .tone = { chirp.startFreq, chirp.duration } } };
			xQueueSend(queue, &item, portMAX_DELAY);
			continue;
		}
		//TODO - group tones with a low  frequency delta, this would reduce the number of discrete tones
		for(uint32_t durationSum = 0; durationSum < chirp.duration;){
			uint16_t freq = freqMap((long) durationSum, 0, chirp.duration, chirp.startFreq, chirp.endFreq);
			uint16_t tonePeriod = std::max(MinimumLength, (uint32_t) (1000.0 / (double) freq));
			uint16_t constrainedTonePeriod = std::min(tonePeriod, (uint16_t) (chirp.duration - durationSum));

			if(!PWM::checkFrequency(freq)){
				freq = 0;
			}
			QueueItem item = { QueueItem::Type::Tone, { .tone = { freq, constrainedTonePeriod } } };
			xQueueSend(queue, &item, portMAX_DELAY);

			durationSum += constrainedTonePeriod;
		}
	}
	const QueueItem endItem = { QueueItem::Type::Tone, { .tone = { 0, 0 } } };
	xQueueSend(queue, &endItem, portMAX_DELAY);
}

/*void IRAM_ATTR ChirpSystem::playFromISR(std::initializer_list<Chirp> sound){
	playFromISR((Sound) sound);
}

void IRAM_ATTR ChirpSystem::playFromISR(const Sound& sound){
	if(mute) return;

	uint32_t totalLength = 0;
	for(const Chirp& chirp : sound){
		totalLength += chirp.duration;
	}
	if(totalLength > MaxLength){
		return;
	}

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(!xQueueIsQueueEmptyFromISR(queue)){
		QueueItem item = { QueueItem::Type::ClearTones, { .numToClear = (uint16_t) uxQueueMessagesWaitingFromISR(queue) } };
		xQueueSendToFrontFromISR(queue, &item, &xHigherPriorityTaskWoken);
	}

	for(const Chirp& chirp : sound){
		if(chirp.endFreq == chirp.startFreq){
			QueueItem item = { QueueItem::Type::Tone, { .tone = { chirp.startFreq, chirp.duration } } };
			xQueueSendFromISR(queue, &item, &xHigherPriorityTaskWoken);
			continue;
		}
		//TODO - group tones with a low  frequency delta, this would reduce the number of discrete tones
		for(uint32_t durationSum = 0; durationSum < chirp.duration;){
			uint16_t freq = freqMap((long) durationSum, 0, chirp.duration, chirp.startFreq, chirp.endFreq);
			uint16_t tonePeriod = std::max(MinimumLength, (uint32_t) (1000.0 / (double) freq));
			uint16_t constrainedTonePeriod = std::min(tonePeriod, (uint16_t) (chirp.duration - durationSum));

			if(!PWM::checkFrequency(freq)){
				freq = 0;
			}
			QueueItem item = { QueueItem::Type::Tone, { .tone = { freq, constrainedTonePeriod } } };
			xQueueSendFromISR(queue, &item, &xHigherPriorityTaskWoken);

			durationSum += constrainedTonePeriod;
		}
	}

	const QueueItem endItem = { QueueItem::Type::Tone, { .tone = { 0, 0 } } };
	xQueueSendFromISR(queue, &endItem, &xHigherPriorityTaskWoken);
}*/

void IRAM_ATTR ChirpSystem::stopFromISR(){
	BaseType_t priority = pdFALSE;

	if(!xQueueIsQueueEmptyFromISR(queue)){
		QueueItem item = { QueueItem::Type::ClearTones, { .numToClear = (uint16_t) uxQueueMessagesWaitingFromISR(queue) } };
		xQueueSendToFrontFromISR(queue, &item, &priority);
	}
}

void ChirpSystem::stop(){
	xQueueReset(queue);
	pwm.stop();
	detach();
}

void ChirpSystem::setMute(bool mute){
	this->mute = mute;
	if(mute){
		stop();
	}
}

bool ChirpSystem::isMuted() const{
	return mute;
}

void ChirpSystem::setPersistentAttach(bool persistent){
	this->pwmPersistence = persistent;
	if(persistent){
		attach();
	}else if(uxQueueMessagesWaiting(queue) <= 0){
		detach();
	}
}

void ChirpSystem::attach(){
	if(attached) return;
	attached = true;
	pwm.attach();
	pwm.setDuty(50);
	sleepLock.acquire();
}

void ChirpSystem::detach(){
	if(!attached) return;
	attached = false;
	pwm.detach();
	sleepLock.release();
}

void IRAM_ATTR ChirpSystem::isr(void* arg){
	BaseType_t priority = pdFALSE;
	xSemaphoreGiveFromISR(arg, &priority);
}

void ChirpSystem::loop(){
	while(!xSemaphoreTake(timerSem, portMAX_DELAY));
	timer.stop();

	QueueItem item{};
	if(xQueueReceive(queue, &item, portMAX_DELAY)){

		while(item.type == QueueItem::Type::ClearTones || (item.type == QueueItem::Type::Tone && item.data.tone.length == 0)){
			if(item.type == QueueItem::Type::ClearTones){
				processClearTone(item.data.numToClear);
			}else if(item.type == QueueItem::Type::Tone){
				if(item.data.tone.freq == 0){
					pwm.stop();
					if(!pwmPersistence){
						detach();
					}
				}
			}
			xQueueReceive(queue, &item, portMAX_DELAY);
		}

		auto tone = item.data.tone;

		if(tone.freq == 0){
			pwm.stop();
		}else{
			pwm.setFreq(tone.freq);
		}

		timer.setPeriod(tone.length);
		timer.start();
	}
}

void ChirpSystem::processClearTone(uint16_t numToClear){
	const auto available = uxQueueMessagesWaiting(queue);
	if(available < numToClear){
		ESP_LOGE(TAG, "Failed to clear %d items from queue! Only available: %d items.", available, numToClear);
	}
	QueueItem item{};
	for(uint16_t i = 0; i < numToClear; i++){
		if(!xQueueReceive(queue, &item, 0)){
			ESP_LOGE(TAG, "Failed to clear %d items from queue! Only cleared: %d items.", available, i);
			return;
		}
	}
}

constexpr long ChirpSystem::freqMap(long val, long fromLow, long fromHigh, long toLow, long toHigh){
	return (val - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}
