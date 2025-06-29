#include "Battery.h"
#include "Pins.hpp"
#include "Util/Events.h"
#include <soc/efuse_reg.h>
#include <Util/stdafx.h>
#include <cmath>
#include <esp_log.h>
#include <driver/gpio.h>
#include "Util/Services.h"
#include "Services/SleepMan.h"

static const char* TAG = "Battery";

Battery::Battery() : Threaded("Battery", 3 * 1024, 5, 1), chargeHyst(500, ChargingState::Unplugged), sem(xSemaphoreCreateBinary()), timer(ShortMeasureIntverval, isr, sem){
	gpio_config_t cfg_gpio = {};
	cfg_gpio.mode = GPIO_MODE_INPUT;
	cfg_gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
	cfg_gpio.pull_up_en = GPIO_PULLUP_DISABLE;
	cfg_gpio.pin_bit_mask = 1ULL << Pins::get(Pin::Usb);
	cfg_gpio.intr_type = GPIO_INTR_POSEDGE;
	ESP_ERROR_CHECK(gpio_config(&cfg_gpio));

	cfg_gpio.intr_type = GPIO_INTR_DISABLE;
	cfg_gpio.pin_bit_mask = 1ULL << Pins::get(Pin::ChrgIn);
	gpio_config(&cfg_gpio);
	cfg_gpio.pin_bit_mask = 1ULL << Pins::get(Pin::ChrgOut);
	gpio_config(&cfg_gpio);
}

Battery::~Battery(){
	gpio_set_intr_type((gpio_num_t) Pins::get(Pin::Usb), GPIO_INTR_DISABLE);
	gpio_isr_handler_remove((gpio_num_t) Pins::get(Pin::Usb));

	timer.stop();
	stop(0);
	abortFlag = true;
	xSemaphoreGive(sem);
	while(running()){
		vTaskDelay(1);
	}
}

void Battery::begin(){
	start();
	startTimer();
	gpio_isr_handler_add((gpio_num_t) Pins::get(Pin::Usb), isr, sem);
}

void Battery::checkCharging(bool fresh){
	if(shutdown) return;

	auto plugin = gpio_get_level((gpio_num_t) Pins::get(Pin::Usb)) == 1;
	bool chrg = gpio_get_level((gpio_num_t) Pins::get(Pin::ChrgIn)) == 0;

	ESP_LOGD(TAG, "plugged in: %d, charging: %d", plugin, chrg);

	ChargingState newState;
	if(!plugin){
		newState = ChargingState::Unplugged;
	}else{
		//Prevent transition from Full to Charging while plugged during current spikes.
		if(chargeHyst.get() == ChargingState::Full){
			newState = ChargingState::Full;
		}else{
			newState = chrg ? ChargingState::Charging : ChargingState::Full;
		}
	}

	if(fresh){
		chargeHyst.reset(newState);
	}else{
		chargeHyst.update(newState);
	}

	if(chargeHyst.get() != lastCharging){
		lastCharging = chargeHyst.get();
		sample(true);
		Events::post(Facility::Battery, Battery::Event{ .action = Event::Charging, .chargeStatus = lastCharging });
	}
}

void Battery::loop(){
	while(!xSemaphoreTake(sem, portMAX_DELAY)){
		timer.stop();
		startTimer();
	}
	timer.stop();

	if(abortFlag || shutdown) return;

	std::lock_guard lock(mut);

	checkCharging();
	if(sleep){
		ESP_LOGI(TAG, "InSleepReconfigure\n");
		inSleepReconfigure();
		sample(true);
	}else{
		sample();
	}

	startTimer();
}

void Battery::startTimer(){
	timer.stop();
	if(shutdown) return;

	if((getChargingState() != ChargingState::Unplugged) || !sleep){
		timer.setPeriod(ShortMeasureIntverval);
	}else{
		timer.setPeriod(LongMeasureIntverval);
	}
	timer.start();
}

void IRAM_ATTR Battery::isr(void* arg){
	BaseType_t priority = pdFALSE;
	xSemaphoreGiveFromISR(arg, &priority);
}

Battery::ChargingState Battery::getChargingState() const{
	return chargeHyst.get();
}

bool Battery::isShutdown() const{
	return shutdown;
}

void Battery::setSleep(bool sleep){
	if(!sleep){
		ESP_LOGI(TAG, "Battery reconfigure on wake\n");
		inSleepReconfigure();
	}

	timer.stop();
	std::lock_guard lock(mut);

	onSleep(sleep);

	this->sleep = sleep;
	xSemaphoreGive(sem);
}