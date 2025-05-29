#include "Battery.h"
#include "../Pins.hpp"
#include "Util/Events.h"
#include <soc/efuse_reg.h>
#include <Util/stdafx.h>
#include <cmath>
#include <driver/gpio.h>
#include "Util/Services.h"
#include "Services/SleepMan.h"

static const char* TAG = "Battery";

Battery::Battery(ADC& adc) : Threaded("Battery", 3 * 1024, 5, 1),
						#if CONFIG_VERSION == 0x0007
							adc(adc), refSwitch(PIN_VREF),
						#endif
							hysteresis({ 0, 4, 15, 30, 70, 100 }, 3), chargeHyst(500, ChargingState::Unplugged), sem(xSemaphoreCreateBinary()), timer(ShortMeasureIntverval, isr, sem){
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

#if CONFIG_VERSION == 0x0007
	const auto config = [this, &adc](int pin, adc_cali_handle_t& cali, std::unique_ptr<ADCReader>& reader, bool emaAndMap){
		adc_unit_t unit;
		adc_channel_t chan;
		ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &chan));
		assert(unit == adc.getUnit());

		adc.config(chan, {
				.atten = ADC_ATTEN_DB_0,
				.bitwidth = ADC_BITWIDTH_12
		});

		const adc_cali_curve_fitting_config_t curveCfg = {
				.unit_id = unit,
				.chan = chan,
				.atten = ADC_ATTEN_DB_0,
				.bitwidth = ADC_BITWIDTH_12
		};
		ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curveCfg, &cali));

		if(emaAndMap){
			reader = std::make_unique<ADCReader>(adc, chan, caliBatt, Offset, Factor, EmaA, VoltEmpty, VoltFull);
		}else{
			reader = std::make_unique<ADCReader>(adc, chan, caliBatt, Offset, Factor);
		}
	};

	config(PIN_BATT, caliBatt, readerBatt, true);
	config(PIN_BATT, caliRef, readerRef, false);

	calibrate();

	sample(true);
#elif CONFIG_VERSION == 0x0003
	adc_unit_t unit;
	adc_channel_t chan;
	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(PIN_BATT, &unit, &chan));
	assert(unit == adc.getUnit());

	adc.config(chan, {
			.atten = ADC_ATTEN_DB_11,
			.bitwidth = ADC_BITWIDTH_12
	});

	const adc_cali_curve_fitting_config_t curveCfg = {
			.unit_id = unit,
			.chan = chan,
			.atten = ADC_ATTEN_DB_11,
			.bitwidth = ADC_BITWIDTH_12
	};
	ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curveCfg, &caliBatt));

	readerBatt = std::make_unique<ADCReader>(adc, chan, caliBatt, 0, 2.0f, EmaA, VoltEmpty, VoltFull);

	readerBatt->resetEma();
	if(readerBatt->getValue() <= 1.f){
		auto sleepMan = (SleepMan*)Services.get(Service::Sleep);
		sleepMan->shutdown();
	}
#endif

	sample(true);

	checkCharging(true);
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

#if CONFIG_VERSION == 0x0007
void Battery::calibrate(){
	refSwitch.on();

	delayMillis(100);
	for(int i = 0; i < CalReads; i++){
		readerRef->sample();
		delayMillis(10);
	}

	float total = 0;
	for(int i = 0; i < CalReads; i++){
		total += readerRef->sample();
		delayMillis(10);
	}

	const float reading = total / (float) CalReads;
	const float offset = CalExpected - reading;
	readerBatt->setMoreOffset(offset);

	refSwitch.off();

	printf("Calibration: Read %.02f mV, expected %.02f mV. Applying %.02f mV offset.\n", reading, CalExpected, offset);
}
#endif

void Battery::sample(bool fresh){
	if(shutdown) return;
	if(getChargingState() != ChargingState::Unplugged) return;

	auto oldLevel = getLevel();

#if CONFIG_VERSION == 0x0007
	if(fresh){
		readerBatt->resetEma();
		hysteresis.reset(readerRef->getValue());
	}else{
		hysteresis.update(readerRef->sample());
	}
#elif CONFIG_VERSION == 0x0003
	if(fresh){
		readerBatt->resetEma();
		hysteresis.reset(readerBatt->getValue());
	}else{
		auto val = readerBatt->sample();
		hysteresis.update(val);
	}
#endif

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

	readerBatt->setEMAFactor(sleep ? EmaA_sleep : EmaA);

	this->sleep = sleep;
	xSemaphoreGive(sem);
}

uint8_t Battery::getPerc() const{
	return readerBatt->getValue();
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
