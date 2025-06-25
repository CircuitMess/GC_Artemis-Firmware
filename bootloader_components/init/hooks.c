#include "esp_attr.h"
#include "hal/gpio_hal.h"
#include "esp_efuse.h"

static const char* TAG = "hook";

/** Function used to tell the linker to include this file with all its symbols. */
void bootloader_hooks_include(void){}


//Note: This works since LED pins are the same across all revisions.
//Change if necessary (GPIOs could mess up another hardware revision)
static const gpio_num_t LEDs[] = { 17, 18, 43, 44, 45, 46 };

void setLEDs(){
	for(int i = 0; i < sizeof(LEDs)/sizeof(LEDs[0]); i++){
		const uint8_t pin = LEDs[i];
		gpio_ll_pullup_dis(&GPIO, pin);
		gpio_ll_pulldown_dis(&GPIO, pin);
		gpio_ll_input_disable(&GPIO, pin);

		gpio_ll_output_enable(&GPIO, pin);
		gpio_ll_set_level(&GPIO, pin, 0);
	}
}

void IRAM_ATTR bootloader_before_init(void){
	/* Keep in my mind that a lot of functions cannot be called from here
	 * as system initialization has not been performed yet, including
	 * BSS, SPI flash, or memory protection. */
	// ESP_LOGI("HOOK", "This hook is called BEFORE bootloader initialization");


	//WARNING - proceed with caution around PWDN (powerdown) pin!
	//Setting it to low (0) during bootloader WILL brick the device!
	esp_efuse_desc_t RevBlock = { EFUSE_BLK3, 32, 8 };
	const esp_efuse_desc_t* Rev_Blob[] = { &RevBlock, NULL };

	esp_efuse_desc_t PIDBlock = { EFUSE_BLK3, 16, 16 };
	const esp_efuse_desc_t* PID_Blob[] = { &PIDBlock, NULL };

	uint8_t Revision = 0;
    uint16_t PID = 0;
	gpio_num_t PWDN;

	esp_err_t err = esp_efuse_read_field_blob((const esp_efuse_desc_t**) Rev_Blob, &Revision, 8);
    if(err != ESP_OK){
    	ESP_LOGE(TAG, "Failed to read revision");
        return;
    }

	err = esp_efuse_read_field_blob((const esp_efuse_desc_t**) PID_Blob, &PID, 16);
	if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to read PID");
		return;
	}

	if(PID == 0 && Revision == 0){
		PWDN = GPIO_NUM_37;
		ESP_LOGI(TAG, "Rev2 assumed from bootloader");
    }else if(Revision == 2){
		PWDN = GPIO_NUM_37;
		ESP_LOGI(TAG, "Rev2 from bootloader");
	}else{
		PWDN = GPIO_NUM_42;
		ESP_LOGI(TAG, "Rev1 from bootloader");
	}

	gpio_ll_input_enable(&GPIO, PWDN);
	gpio_ll_pulldown_dis(&GPIO, PWDN);
	gpio_ll_pullup_dis(&GPIO, PWDN);

	setLEDs();
}

void bootloader_after_init(void){
	// ESP_LOGI("HOOK", "This hook is called AFTER bootloader initialization");
}
