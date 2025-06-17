#ifndef ARTEMIS_BATTERYV3_H
#define ARTEMIS_BATTERYV3_H

#include "Periph/ADC.h"
#include "Util/Hysteresis.h"
#include "Services/ADCReader.h"
#include "Periph/PinOut.h"
#include <esp_efuse.h>
#include <memory>
#include "Battery.h"

class BatteryV3 : public Battery {
public:
	BatteryV3(ADC& adc);
	virtual ~BatteryV3() override;

	void setSleep(bool sleep);

	uint8_t getPerc() const;
	Level getLevel() const;

private:
	static constexpr float VoltFull = 4200.0f; //[mV]
	static constexpr float VoltEmpty = 3600.0f; //[mV]
	static constexpr float EmaA = 0.05f;
	static constexpr float EmaA_sleep = 0.5f;

	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 0;
	static constexpr int CalReads = 10;
	static constexpr float CalExpected = 2500;

	PinOut refSwitch;

	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

	std::unique_ptr<ADCReader> readerRef;
	adc_cali_handle_t caliRef;

	void calibrate();

	virtual void sample(bool fresh) override;

	virtual void onSleep(bool sleep) override;
};

#endif //ARTEMIS_BATTERYV3_H