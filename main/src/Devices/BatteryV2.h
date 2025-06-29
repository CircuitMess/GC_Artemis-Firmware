#ifndef ARTEMIS_BATTERYV2_H
#define ARTEMIS_BATTERYV2_H

#include "Periph/ADC.h"
#include "Util/Hysteresis.h"
#include "Services/ADCReader.h"
#include <esp_efuse.h>
#include <memory>
#include "Battery.h"

class BatteryV2 : public Battery {
public:
	BatteryV2(ADC& adc);
	virtual ~BatteryV2() override;

	virtual uint8_t getPerc() const override;
	virtual Level getLevel() const override;

private:
	static constexpr float VoltFull = 4200.0f; //[mV]
	static constexpr float VoltEmpty = 3600.0f; //[mV]
	static constexpr float EmaA = 0.05f;
	static constexpr float EmaA_sleep = 0.5f;

	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

	virtual void sample(bool fresh) override;

	static constexpr esp_efuse_desc_t adc1_low = { EFUSE_BLK3, 0, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_low[] = { &adc1_low, nullptr };
	static constexpr esp_efuse_desc_t adc1_high = { EFUSE_BLK3, 8, 8 };
	static constexpr const esp_efuse_desc_t* efuse_adc1_high[] = { &adc1_high, nullptr };

	virtual void onSleep(bool sleep) override;
	void inSleepReconfigure() override;
};

#endif //ARTEMIS_BATTERYV2_H
