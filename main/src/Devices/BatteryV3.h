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

	virtual uint8_t getPerc() const override;
	virtual Level getLevel() const override;

private:
	static constexpr float VoltFull = 4150.0f; //[mV]
	static constexpr float VoltEmpty = 3600.0f; //[mV]
	static constexpr float EmaA = 0.05f;
	static constexpr float EmaA_sleep = 0.5f;

	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 0;

	/**
	 * Voltage drop on battery line!
	 * Measured with full battery on max brightness, and on battery plugged out.
	 * Voltage drop is approximately linear across 3.6 - 4.2V range.
	 * Sample size of 2 devices.
	 */
	static constexpr float BattReadOffset = 75; //[mV],
	static constexpr int CalReads = 10;
	static constexpr float CalExpected = 2500;

	ADC& adc;
	PinOut refSwitch;

	Hysteresis hysteresis;

	std::unique_ptr<ADCReader> readerBatt;
	adc_cali_handle_t caliBatt;

	std::unique_ptr<ADCReader> readerRef;
	adc_cali_handle_t caliRef;
	float lastCalibrationOffset = 0;

	void calibrate();

	virtual void sample(bool fresh) override;

	virtual void onSleep(bool sleep) override;

	void configReader(int pin, adc_cali_handle_t& cali, std::unique_ptr<ADCReader>& reader, bool emaAndMap);

	void inSleepReconfigure() override;
};

#endif //ARTEMIS_BATTERYV3_H