#include "ADC.h"
#include <driver/adc.h>
#include <esp_log.h>
#include <algorithm>
#include <glm.hpp>

static const char* TAG = "ADC";

ADC::ADC(gpio_num_t pin, float ema_a, int min, int max, int readingOffset, float factor1, float factor2) : pin(pin), ema_a(ema_a), min(min), max(max), offset(readingOffset), factor1(factor1), factor2(factor2){
	if(pin != GPIO_NUM_6){
		ESP_LOGE(TAG, "Only GPIO 36 is supported for ADC");
		valid = false;
		return;
	}

	if(pin == GPIO_NUM_6){
		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
	}

	sample();
}

void ADC::setEmaA(float emaA){
	ema_a = emaA;
}

void ADC::resetEma(){
	value = -1;
	sample();
}

float ADC::sample(){
	if(!valid){
		return 0;
	}

	float reading = 0;
	if(pin == GPIO_NUM_6){
		reading = adc1_get_raw(ADC1_CHANNEL_5);
	}

	if(value == -1 || ema_a == 1){
		value = reading;
	}else{
		value = value * (1.0f - ema_a) + ema_a * reading;
	}

	return getVal();
}

float ADC::getVal() const{
	if(!valid){
		return 0;
	}

	const float adjusted = offset + value * factor1 + glm::pow(value, 2.0f) * factor2;

	if(max == 0 && min == 0){
		return adjusted;
	}

	float minimum = this->min;
	float maximum = this->max;
	bool inverted = minimum > maximum;
	if(inverted){
		std::swap(minimum, maximum);
	}

	float val = std::clamp(adjusted, minimum, maximum);
	val = (val - minimum) / (maximum - minimum);
	val = std::clamp(val*100.0f, 0.0f, 100.0f);

	if(inverted){
		val = 100.0f - val;
	}

	return val;
}

