#include "BatteryV3.h"
#include "Pins.hpp"
#include "Util/Events.h"
#include <soc/efuse_reg.h>
#include <Util/stdafx.h>
#include <cmath>
#include <driver/gpio.h>
#include "Services/SleepMan.h"
#include <Util/Services.h>

static const char* TAG = "Battery";

BatteryV3::BatteryV3(ADC& adc) : adc(adc), refSwitch(Pins::get(Pin::BattVref)), hysteresis({ 0, 4, 15, 30, 70, 100 }, 3){
	configReader(Pins::get(Pin::BattRead), caliBatt, readerBatt, true);
	configReader(Pins::get(Pin::BattRead), caliRef, readerRef, false);

	calibrate();

	readerBatt->resetEma();
	if(readerBatt->getValue() <= 1.f){
		auto sleepMan = (SleepMan*)Services.get(Service::Sleep);
		sleepMan->shutdown();
	}

	sample(true);

	checkCharging(true);
}

BatteryV3::~BatteryV3(){

}

void BatteryV3::calibrate(){
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
	delayMillis(100);

	lastCalibrationOffset = offset;

	printf("Calibration: Read %.02f mV, expected %.02f mV. Applying %.02f mV offset.\n", reading, CalExpected, offset);
}

void BatteryV3::sample(bool fresh){
	if(Battery::isShutdown()) return;
	if(Battery::getChargingState() != ChargingState::Unplugged) return;

	auto oldLevel = getLevel();

	if(fresh){
		readerBatt->resetEma();
		const float val = readerBatt->getValue();
		hysteresis.reset(val);
	}else{
		const float val = readerBatt->sample();
		hysteresis.update(val);
	}

	if(oldLevel != getLevel() || fresh){
		Events::post(Facility::Battery, Battery::Event{ .action = Event::LevelChange, .level = getLevel() });
	}

	if(getLevel() == Critical){
		setShutdown(true);
		extern void shutdown();
		shutdown();
	}
}

void BatteryV3::onSleep(bool sleep) {
	readerBatt->setEMAFactor(sleep ? EmaA_sleep : EmaA);
}

uint8_t BatteryV3::getPerc() const{
	return readerBatt->getValue();
}

Battery::Level BatteryV3::getLevel() const{
	return (Level) hysteresis.get();
}

void BatteryV3::inSleepReconfigure(){
	adc.reinit();
	adc_cali_delete_scheme_curve_fitting(caliBatt);
	configReader(Pins::get(Pin::BattRead), caliBatt, readerBatt, true);
	readerBatt->setMoreOffset(lastCalibrationOffset);
}

void BatteryV3::configReader(int pin, adc_cali_handle_t& cali, std::unique_ptr<ADCReader>& reader, bool emaAndMap){
	adc_unit_t unit;
	adc_channel_t chan;
	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &chan));
	assert(unit == adc.getUnit());

	adc.config(chan, {
			.atten = ADC_ATTEN_DB_2_5,
			.bitwidth = ADC_BITWIDTH_12
	});

	const adc_cali_curve_fitting_config_t curveCfg = {
			.unit_id = unit,
			.chan = chan,
			.atten = ADC_ATTEN_DB_2_5,
			.bitwidth = ADC_BITWIDTH_12
	};
	ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curveCfg, &cali));

	if(emaAndMap){
		reader = std::make_unique<ADCReader>(adc, chan, caliBatt, BattReadOffset, Factor, EmaA, VoltEmpty, VoltFull);
	}else{
		reader = std::make_unique<ADCReader>(adc, chan, caliBatt, Offset, Factor);
	}
}
