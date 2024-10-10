#include <esp_clk_tree.h>
#include <esp_log.h>
#include "PWM.h"

static const char* TAG = "PMW";

PWM::PWM(uint8_t pin, ledc_channel_t channel, bool invertDuty) : pin(pin), channel(channel), invertDuty(invertDuty){

	ledc_timer_config_t ledc_timer = {
			.speed_mode       = getSpeedMode(channel),
			.duty_resolution  = DutyResDefault,
			.timer_num        = getTimer(channel),
			.freq_hz          = DefaultFreq,
			.clk_cfg          = LEDC_AUTO_CLK,
			// .deconfigure      = false
	};
	if(ledc_timer_config(&ledc_timer) != ESP_OK){
		ESP_LOGE(TAG, "timer config failed!");
		return;
	}
	attach();
}

PWM::~PWM(){
	stop();
	detach();
}

void PWM::setFreq(uint16_t freq){
	if(pin == (uint8_t) -1) return;


	if(!checkFrequency(freq)){
		ESP_LOGW(TAG, "couldnt write frequency %d because of clock divisor limitations\n", freq);
		return;
	}

	auto group = getSpeedMode(channel);
	auto timer = getTimer(channel);

	ledc_set_freq(group, timer, freq);
	ledc_update_duty(group, channel);
}

void PWM::setDuty(uint8_t perc){
	attach();
	uint32_t duty = (FullDuty * perc) / 100;
	auto group = getSpeedMode(channel);
	ledc_set_duty(group, channel, duty);
	ledc_update_duty(group, channel);
}

void PWM::stop(){
	if(pin == (uint8_t) -1) return;
	ledc_stop(getSpeedMode(channel), channel, invertDuty);
}

void PWM::attach(){
	if(pin == (uint8_t) -1) return;
	if(attached) return;

	ledc_channel_config_t ledc_channel = {
			.gpio_num       = pin,
			.speed_mode     = getSpeedMode(channel),
			.channel        = channel,
			.intr_type      = LEDC_INTR_DISABLE,
			.timer_sel      = getTimer(channel),
			.duty           = 0,
			.hpoint         = 0,
			.flags = { .output_invert = invertDuty }
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	attached = true;
}

void PWM::detach(){
	if(!attached) return;
	gpio_config_t cfg = {
			.pin_bit_mask = 1ULL << pin,
			.mode = GPIO_MODE_OUTPUT
	};
	gpio_config(&cfg);
	gpio_set_level((gpio_num_t) pin, invertDuty);
	attached = false;
}

constexpr ledc_mode_t PWM::getSpeedMode(ledc_channel_t channel){
	return static_cast<ledc_mode_t>((channel / 8));
}

constexpr ledc_timer_t PWM::getTimer(ledc_channel_t channel){
	return static_cast<ledc_timer_t>(((channel / 2) % 4));
}
