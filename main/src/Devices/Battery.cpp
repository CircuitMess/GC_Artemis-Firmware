#include "Battery.h"
#include "../Pins.hpp"
#include "Util/Events.h"
#include <soc/efuse_reg.h>
#include <Util/stdafx.h>
#include <cmath>
#include <driver/gpio.h>

static const char* TAG = "Battery";

#define MAX_READ 2505 // 4.2V
#define MIN_READ 2120 // 3.6V

Battery::Battery() : Threaded("Battery", 3 * 1024, 5), adc((gpio_num_t) PIN_BATT, 0.05, MIN_READ, MAX_READ, getVoltOffset()),
					 hysteresis({ 0, 4, 15, 30, 70, 100 }, 3),
					 chargeHyst(500, ChargingState::Unplugged), sem(xSemaphoreCreateBinary()), timer(ShortMeasureIntverval, isr, sem){

	gpio_config_t cfg_gpio = {};
	cfg_gpio.mode = GPIO_MODE_INPUT;
	cfg_gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
	cfg_gpio.pull_up_en = GPIO_PULLUP_DISABLE;
	cfg_gpio.pin_bit_mask = 1ULL << PIN_USB;
	cfg_gpio.intr_type = GPIO_INTR_POSEDGE;
	ESP_ERROR_CHECK(gpio_config(&cfg_gpio));

	cfg_gpio.intr_type = GPIO_INTR_DISABLE;
	cfg_gpio.pin_bit_mask = 1ULL << PIN_CHRGIN;
	gpio_config(&cfg_gpio);
	cfg_gpio.pin_bit_mask = 1ULL << PIN_CHRGOUT;
	gpio_config(&cfg_gpio);

	checkCharging(true);
	sample(true); // this will initiate shutdown if battery is critical
}

Battery::~Battery(){
	gpio_set_intr_type((gpio_num_t) PIN_USB, GPIO_INTR_DISABLE);
	gpio_isr_handler_remove((gpio_num_t) PIN_USB);

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
	gpio_isr_handler_add((gpio_num_t) PIN_USB, isr, sem);
}

uint16_t Battery::mapRawReading(uint16_t reading){
	return std::round(map((double) reading, MIN_READ, MAX_READ, 3600, 4200));
}

int16_t Battery::getVoltOffset(){
	int16_t upper = 0, lower = 0;
	esp_efuse_read_field_blob((const esp_efuse_desc_t**) efuse_adc1_low, &lower, 8);
	esp_efuse_read_field_blob((const esp_efuse_desc_t**) efuse_adc1_high, &upper, 8);
	return (upper << 8) | lower;
}

void Battery::checkCharging(bool fresh){
	if(shutdown) return;

	auto plugin = gpio_get_level((gpio_num_t) PIN_USB) == 1;
	bool chrg = gpio_get_level((gpio_num_t) PIN_CHRGIN) == 0;

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

void Battery::sample(bool fresh){
	if(shutdown) return;
	if(getChargingState() != ChargingState::Unplugged) return;

	auto oldLevel = getLevel();

	if(fresh){
		adc.resetEma();
		hysteresis.reset(adc.getVal());
	}else{
		auto val = adc.sample();
		hysteresis.update(val);
	}

	if(oldLevel != getLevel() || fresh){
		Events::post(Facility::Battery, Battery::Event{ .action = Event::LevelChange, .level = getLevel() });
	}

	if(getLevel() == Critical){
		shutdown = true;
		extern void shutdown();
		shutdown();
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
	sample();

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

void Battery::setSleep(bool sleep){
	timer.stop();
	std::lock_guard lock(mut);

	adc.setEmaA(sleep ? 0.5 : 0.05);

	this->sleep = sleep;
	xSemaphoreGive(sem);
}

uint8_t Battery::getPerc() const{
	return adc.getVal();
}

Battery::Level Battery::getLevel() const{
	return (Level) hysteresis.get();
}

Battery::ChargingState Battery::getChargingState() const{
	return chargeHyst.get();
}

bool Battery::isShutdown() const{
	return shutdown;
}
