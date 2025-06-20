#include "ADCReader.h"
#include <algorithm>

ADCReader::ADCReader(ADC& adc, adc_channel_t chan, adc_cali_handle_t cali, float offset, float factor, float emaA, float min, float max)
		: adc(adc), chan(chan), cali(cali), offset(offset), factor(factor), emaA(emaA), min(min), max(max){

}

float ADCReader::sample(){
	int raw = 0;
	if(adc.read(chan, raw, cali) != ESP_OK){
		return getValue();
	}

	if(value == -1 || emaA == 1){
		value = raw;
	}else{
		value = value * (1.0f - emaA) + emaA * (float) raw;
	}

	return getValue();
}

float ADCReader::getValue() const{
	const float adjusted = value * factor + offset + moreOffset;

	if(max == 0 && min == 0){
		return adjusted;
	}

	float minimum = min;
	float maximum = max;

	if(min > max){
		std::swap(minimum, maximum);
	}

	float val = std::clamp(adjusted, minimum, maximum);
	val = (val - minimum) / (maximum - minimum);
	val = std::clamp(val * 100.0f, 0.0f, 100.0f);

	if(min > max){
		val = 100.0f - val;
	}

	return val;
}

void ADCReader::resetEma(){
	value = -1;
	sample();
}

void ADCReader::setMoreOffset(float offset){
	moreOffset = offset;
}

void ADCReader::setEMAFactor(float factor){
	emaA = factor;
	resetEma();
}
