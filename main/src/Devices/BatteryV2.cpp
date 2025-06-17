#include "BatteryV2.h"
#include "Pins.hpp"
#include "Util/Events.h"
#include <soc/efuse_reg.h>
#include <driver/gpio.h>
#include "Util/Services.h"
#include "Services/SleepMan.h"

static const char* TAG = "Battery";

BatteryV2::BatteryV2(ADC& adc) : hysteresis({ 0, 4, 15, 30, 70, 100 }, 3) {
	adc_unit_t unit;
	adc_channel_t chan;
	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(Pins::get(Pin::BattRead), &unit, &chan));
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

	sample(true);

	checkCharging(true);
}

BatteryV2::~BatteryV2(){

}

void BatteryV2::sample(bool fresh){
	if(isShutdown()) return;
	if(getChargingState() != ChargingState::Unplugged) return;

	auto oldLevel = getLevel();

	if(fresh){
		readerBatt->resetEma();
		hysteresis.reset(readerBatt->getValue());
	}else{
		auto val = readerBatt->sample();
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

void BatteryV2::onSleep(bool sleep) {
	readerBatt->setEMAFactor(sleep ? EmaA_sleep : EmaA);
}

uint8_t BatteryV2::getPerc() const{
	return readerBatt->getValue();
}

Battery::Level BatteryV2::getLevel() const{
	return (Level) hysteresis.get();
}
