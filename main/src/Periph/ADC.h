#ifndef CLOCKSTAR_FIRMWARE_ADC_H
#define CLOCKSTAR_FIRMWARE_ADC_H

#include <hal/gpio_types.h>

class ADC {
public:
	// Specifying min and max maps value to [-100, +100]
	ADC(gpio_num_t pin, float ema_a = 1, int min = 0, int max = 0, int readingOffset = 0, float factor1 = 1.0f, float factor2 = 0.0f);

	// Take a sample and get current value
	float sample();

	// Get current value without sampling
	float getVal() const;

	void resetEma();
	void setEmaA(float emaA);

private:
	bool valid = true;

	const gpio_num_t pin;
	float ema_a;
	const float min, max;
	const float offset;
	const float factor1, factor2;

	float value = -1;
};


#endif //CLOCKSTAR_FIRMWARE_ADC_H
