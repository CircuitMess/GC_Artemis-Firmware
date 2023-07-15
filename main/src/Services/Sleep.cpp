#include "Sleep.h"
#include "Pins.hpp"
#include "BLE/ConMan.h"
#include <esp_sleep.h>
#include <esp_pm.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

static const char* TAG = "Sleep";

Sleep::Sleep(Input& input, Time& time) : input(input), time(time){
	confPM(false);
	wakeSem = xSemaphoreCreateBinary();
}

void Sleep::sleep(std::function<void()> preWake){
	ESP_LOGI(TAG, "Goint to sleep\n");

	input.pause();
	time.pause();

	gpio_set_level((gpio_num_t) PIN_BL, 1); // TODO: PWM
	ConMan.goLowPow();

	int64_t sleepStartTime = esp_timer_get_time();
	sleepStart();
	auto sleepTime = esp_timer_get_time() - sleepStartTime;

	ConMan.goHiPow();

	preWake();
	gpio_set_level((gpio_num_t) PIN_BL, 0); // TODO: PWM

	input.resume();
	time.resume();

	ESP_LOGI(TAG, "Slept for %lld us\n", sleepTime);
}

void IRAM_ATTR Sleep::sleepStart(){
	gpio_config_t io_conf = {
			.pin_bit_mask = 1ULL << BTN_ALT,
			.mode = GPIO_MODE_INPUT,
			.pull_up_en = GPIO_PULLUP_DISABLE,
			.pull_down_en = GPIO_PULLDOWN_ENABLE,
			.intr_type = GPIO_INTR_HIGH_LEVEL
	};
	gpio_config(&io_conf);
	gpio_isr_handler_add(WakePin, intr, &wakeSem);

	confPM(true);
	xSemaphoreTake(wakeSem, portMAX_DELAY);
	gpio_isr_handler_remove(WakePin);
	confPM(false);

	gpio_set_intr_type(WakePin, GPIO_INTR_DISABLE);
}

void IRAM_ATTR Sleep::intr(void* arg){
	gpio_set_intr_type(WakePin, GPIO_INTR_POSEDGE);
	auto sem = (SemaphoreHandle_t*) arg;
	xSemaphoreGive(*sem);
}

void Sleep::confPM(bool sleep){
	if(sleep){
		esp_sleep_enable_gpio_wakeup();
		gpio_wakeup_enable(WakePin, GPIO_INTR_HIGH_LEVEL);
	}else{
		gpio_wakeup_disable(WakePin);
		esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
	}

	esp_pm_config_t pm_config = {
			.max_freq_mhz = 240, // e.g. 80, 160, 240
			.min_freq_mhz = 240, // e.g. 40
			.light_sleep_enable = sleep // enable light sleep
	};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}