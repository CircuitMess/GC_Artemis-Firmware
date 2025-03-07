#ifndef ARTEMIS_ADCREADER_H
#define ARTEMIS_ADCREADER_H

#include "Periph/ADC.h"

class ADCReader {
public:
	/**
	 * Reads an ADC channel, applies calibration, EMA, factor, offset, and mapping.
	 * First, calibration data is applied, then EMA.
	 * Smoothed value is multiplied by the factor, then offset is applied.
	 * Lastly, the value is mapped from [min, max] to [0, 100].
	 * @param adc ADC unit
	 * @param chan ADC channel
	 * @param cali Calibration data
	 * @param offset Offset
	 * @param factor Multiplication factor
	 * @param emaA EMA factor
	 * @param min Mapping lower bound
	 * @param max Mapping upper bound
	 */
	ADCReader(ADC& adc, adc_channel_t chan, adc_cali_handle_t cali = nullptr, float offset = 0, float factor = 1, float emaA = 1, float min = 0, float max = 0);

	/** Sample and return new value. */
	float sample();

	/** Return current value without sampling. */
	float getValue() const;

	/** Reset EMA and take a fresh sample. */
	void resetEma();

	/** Set additional offset. This offset is applied after initial factor and offset. */
	void setMoreOffset(float offset);

	/**
	 * Set EMA factor. Filter be refreshed afterwards.
	 * @param factor EMA factor
	 */
	void setEMAFactor(float factor);

private:
	ADC& adc;
	const adc_channel_t chan;
	const adc_cali_handle_t cali;

	const float offset;
	const float factor;
	float emaA;
	const float min;
	const float max;

	float value = -1.0f;

	float moreOffset = 0;

};


#endif //ARTEMIS_ADCREADER_H
