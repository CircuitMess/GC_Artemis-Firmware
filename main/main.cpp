#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <bootloader_random.h>
#include <esp_random.h>
#include "Settings/Settings.h"
#include "Pins.hpp"
#include "Periph/I2C.h"
#include "Periph/PinOut.h"
#include "Periph/Bluetooth.h"
#include "Devices/Battery.h"
#include "Devices/Display.h"
#include "Devices/Input.h"
#include "Devices/IMU.h"
#include "BLE/GAP.h"
#include "BLE/Client.h"
#include "BLE/Server.h"
#include "Notifs/Phone.h"
#include "LV_Interface/LVGL.h"
#include "LV_Interface/FSLVGL.h"
#include "LV_Interface/InputLVGL.h"
#include <lvgl/lvgl.h>
#include "Theme/theme.h"
#include "Util/Services.h"
#include "Services/BacklightBrightness.h"
#include "Services/ChirpSystem.h"
#include "Services/Time.h"
#include "Services/StatusCenter.h"
#include "Services/SleepMan.h"
#include "Screens/ShutdownScreen.h"
#include "Screens/Lock/LockScreen.h"
#include "JigHWTest/JigHWTest.h"
#include "Util/Notes.h"
#include "Screens/Intro/IntroScreen.h"
#include "Filepaths.hpp"
#include "Screens/Lander/LunarLander.h"
#include "Screens/PerseCtrl/WiFiSTA.h"
#include "Screens/PerseCtrl/TCPClient.h"
#include "Util/HWVersion.h"

LVGL* lvgl;
BacklightBrightness* bl;
SleepMan* sleepMan;

void shutdown(){
	if(!lvgl->running()){
		lvgl->start();
	}
	lvgl->startScreen([](){ return std::make_unique<ShutdownScreen>(); });
	lv_timer_handler();
	sleepMan->wake(true);
	if(!bl->isOn()){
		bl->fadeIn();
	}
	vTaskDelay(SleepMan::ShutdownTime-1000);
	sleepMan->shutdown();
}

static const gpio_num_t LEDs[] = { GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_45, GPIO_NUM_46 };

void setLEDs(){
	for(int i = 0; i < sizeof(LEDs) / sizeof(LEDs[0]); i++){
		const gpio_num_t pin = LEDs[i];
		gpio_config_t config = {
				.pin_bit_mask = 1ULL << pin,
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_DISABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
		};

		gpio_config(&config);
		gpio_set_level(pin, 0);
	}
}

void init(){
	setLEDs();
	gpio_install_isr_service(ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);

	auto ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	auto settings = new Settings();
	Services.set(Service::Settings, settings);

	if(JigHWTest::checkJig()){
		printf("Jig\n");
		auto test = new JigHWTest();
		test->start();
		vTaskDelete(nullptr);
	}

	if(!HWVersion::check()){
		while(true){
			vTaskDelay(1000);
			HWVersion::log();
		}
	}

	auto adc1 = new ADC(ADC_UNIT_1);

	auto blPwm = new PWM(PIN_BL, LEDC_CHANNEL_1, true);
	blPwm->detach();
	bl = new BacklightBrightness(blPwm);
	Services.set(Service::Backlight, bl);

	auto buzzPwm = new PWM(PIN_BUZZ, LEDC_CHANNEL_0);
	auto audio = new ChirpSystem(*buzzPwm);
	Services.set(Service::Audio, audio);

	auto i2c = new I2C(I2C_NUM_0, (gpio_num_t) I2C_SDA, (gpio_num_t) I2C_SCL);
	auto imu = new IMU(*i2c);
	Services.set(Service::IMU, imu);

	auto disp = new Display();
	auto input = new Input();
	Services.set(Service::Input, input);

	lvgl = new LVGL(*disp);
	auto theme = theme_init(lvgl->disp());
	lv_disp_set_theme(lvgl->disp(), theme);

	auto lvglInput = new InputLVGL();
	auto fs = new FSLVGL('S');

	sleepMan = new SleepMan(*lvgl);
	Services.set(Service::Sleep, sleepMan);

	auto status = new StatusCenter();
	Services.set(Service::Status, status);

	auto battery = new Battery(*adc1); // Battery is doing shutdown
	if(battery->isShutdown()) return; // Stop initialization if battery is critical
	Services.set(Service::Battery, battery);

	auto rtc = new RTC(*i2c);
	auto time = new Time(*rtc);
	Services.set(Service::Time, time); // Time service is required as soon as Phone is up

	auto bt = new Bluetooth();
	auto gap = new BLE::GAP();
	auto client = new BLE::Client(gap);
	auto server = new BLE::Server(gap);
	auto phone = new Phone(server, client);
	server->start();
	Services.set(Service::Phone, phone);

	srand(esp_random());

	FSLVGL::loadCache(settings->get().themeData.theme);

	// Load start screen here
	lvgl->startScreen([](){ return std::make_unique<IntroScreen>(); });

	// Start UI thread after initialization
	lvgl->start();

	bl->fadeIn();

	if(settings->get().notificationSounds){
		audio->play({
							Chirp{ .startFreq = 250, .endFreq = 250, .duration = 100 },
							Chirp{ .startFreq = 0, .endFreq = 0, .duration = 100 },
							Chirp{ .startFreq = 250, .endFreq = 250, .duration = 100 },
							Chirp{ .startFreq = 0, .endFreq = 0, .duration = 250 },
							Chirp{ .startFreq = 500, .endFreq = 500, .duration = 200 }
					});

		vTaskDelay(2500);
	}

	// Start Battery scanning after everything else, otherwise Critical
	// Battery event might come while initialization is still in progress
	battery->begin();
}

extern "C" void app_main(void){
	init();

	vTaskDelete(nullptr);
}
