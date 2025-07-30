#ifndef ARTEMIS_FIRMWARE_ADC_H
#define ARTEMIS_FIRMWARE_ADC_H

#include <esp_adc/adc_oneshot.h>

class ADC {
public:
	explicit ADC(adc_unit_t unit);
	virtual ~ADC();

	adc_unit_t getUnit() const;

	void config(adc_channel_t chan, const adc_oneshot_chan_cfg_t& cfg);

	esp_err_t read(adc_channel_t chan, int& valueOut, adc_cali_handle_t cali = nullptr) const;

	void reinit();

private:
	adc_oneshot_unit_handle_t hndl;
	const adc_unit_t unit;

};

#endif //ARTEMIS_FIRMWARE_ADC_H
