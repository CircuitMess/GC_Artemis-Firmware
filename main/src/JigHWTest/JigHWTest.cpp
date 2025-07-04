#include "JigHWTest.h"
#include "SPIFFSChecksum.hpp"
#include <Pins.hpp>
#include <soc/efuse_reg.h>
#include <esp_efuse.h>
#include <ctime>
#include <iostream>
#include <esp_mac.h>
#include "Util/Services.h"
#include <driver/gptimer.h>
#include <driver/ledc.h>
#include <Services/ADCReader.h>
#include "Util/Events.h"
#include "Drivers/lsm6ds3tr-c_reg.h"
#include "Util/EfuseMeta.h"
#include <Services/ChirpSystem.h>


JigHWTest* JigHWTest::test = nullptr;
Display* JigHWTest::display = nullptr;
LGFX_Device* JigHWTest::panel = nullptr;
LGFX_Sprite* JigHWTest::canvas = nullptr;
I2C* JigHWTest::i2c = nullptr;
RTC* JigHWTest::rtc = nullptr;
Input* JigHWTest::input = nullptr;


JigHWTest::JigHWTest(){
	display = new Display(EfuseMeta::getHardcodedRevision());
	panel = &display->getLGFX();
	canvas = new LGFX_Sprite(panel);
	canvas->setColorDepth(lgfx::rgb565_2Byte);
	canvas->createSprite(128, 128);

	i2c = new I2C(I2C_NUM_0, (gpio_num_t) Pins::get(Pin::I2cSda), (gpio_num_t) Pins::get(Pin::I2cScl));
	rtc = new RTC(*i2c);
	input = new Input();

	test = this;

	tests.push_back({ JigHWTest::RTCTest, "RTC", [](){}});
	tests.push_back({ JigHWTest::Time1, "RTC crystal", [](){}});
	tests.push_back({ JigHWTest::Time2, "RTC crystal", [](){}});
	tests.push_back({ JigHWTest::IMUTest, "Gyroscope", [](){}});
	tests.push_back({ JigHWTest::IMUInterruptTest, "Gyro interrupt", [](){}});
	tests.push_back({ JigHWTest::SPIFFSTest, "SPIFFS", [](){}});
	tests.push_back({ JigHWTest::BatteryCheck, "Battery check", [](){}});
	tests.push_back({ JigHWTest::VoltReferenceCheck, "Voltage ref", [](){ gpio_set_level((gpio_num_t) Pins::get(Pin::BattVref), 0); }});
//	tests.push_back({ JigHWTest::buttons, "Buttons", [](){}});
	tests.push_back({ JigHWTest::HWVersion, "HW rev", [](){}});
}

bool JigHWTest::checkJig(){
	char buf[7];
	int wp = 0;

	uint32_t start = millis();
	int c;
	while(millis() - start < CheckTimeout){
		vTaskDelay(1);
		c = getchar();
		if(c == EOF) continue;
		buf[wp] = (char) c;
		wp = (wp + 1) % 7;

		for(int i = 0; i < 7; i++){
			int match = 0;
			static const char* target = "JIGTEST";

			for(int j = 0; j < 7; j++){
				match += buf[(i + j) % 7] == target[j];
			}

			if(match == 7) return true;
		}
	}

	return false;
}


void JigHWTest::start(){
	uint64_t _chipmacid = 0LL;
	esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
	printf("\nTEST:begin:%llx\n", _chipmacid);

	gpio_config_t cfg = {
			.pin_bit_mask = ((uint64_t) 1) << Pins::get(Pin::LedBl),
			.mode = GPIO_MODE_OUTPUT,
			.pull_up_en = GPIO_PULLUP_DISABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&cfg);
	gpio_set_level((gpio_num_t) Pins::get(Pin::LedBl), 0);

	canvas->clear(TFT_BLACK);
	canvas->setTextColor(TFT_GOLD);
	canvas->setTextWrap(false, false);
	canvas->setTextDatum(textdatum_t::middle_center);
	canvas->pushSprite(0, 0);

	canvas->setTextFont(0);
	canvas->setTextSize(1);

	canvas->drawString("Artemis Hardware Test", canvas->width() / 2, 6);
	canvas->println();

	canvas->pushSprite(0, 0);

	canvas->setCursor(0, 16);

	bool pass = true;
	for(const Test& test : tests){
		currentTest = test.name;

		canvas->setTextColor(TFT_WHITE);
		canvas->printf("%s: ", test.name);
		canvas->pushSprite(0, 0);


		printf("TEST:startTest:%s\n", currentTest);

		bool result = test.test();

		canvas->setTextColor(result ? TFT_SILVER : TFT_ORANGE);
		canvas->printf("%s\n", result ? "PASS" : "FAIL");
		canvas->pushSprite(0, 0);

		printf("TEST:endTest:%s\n", result ? "pass" : "fail");

		if(!(pass &= result)){
			if(test.onFail){
				test.onFail();
			}

			break;
		}
	}

	if(pass){
		printf("TEST:passall\n");
	}else{
		printf("TEST:fail:%s\n", currentTest);
	}


	//------------------------------------------------------
	canvas->print("\n");
	canvas->setTextColor(pass ? TFT_BLUE : TFT_ORANGE);
	canvas->drawCentreString(pass ? "All OK!" : "FAIL!", canvas->width() / 2, canvas->getCursorY());
	canvas->pushSprite(0, 0);

	bool painted = false;
	const auto color = pass ? TFT_GREEN : TFT_RED;
	auto flashTime = 0;
	const uint16_t note = 1047 + ((rand() * 20) % 400) - 200; //NOTE_C6 = 1047


	auto buzzPwm = new PWM(Pins::get(Pin::Buzz), LEDC_CHANNEL_0);
	auto audio = new ChirpSystem(*buzzPwm);


	static const int LEDs[] = { Pins::get(Pin::Led_1), Pins::get(Pin::Led_2), Pins::get(Pin::Led_3), Pins::get(Pin::Led_4), Pins::get(Pin::Led_5), Pins::get(Pin::Led_6) };
	static const int RGBs[] = { Pins::get(Pin::Rgb_r), Pins::get(Pin::Rgb_g), Pins::get(Pin::Rgb_b) };

	for(int LED : LEDs){
		gpio_config_t cfg = {
				.pin_bit_mask = ((uint64_t) 1) << LED,
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_DISABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
		};
		gpio_config(&cfg);
	}

	for(int LED : RGBs){
		gpio_config_t cfg = {
				.pin_bit_mask = ((uint64_t) 1) << LED,
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_ENABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
		};
		gpio_config(&cfg);
		gpio_set_level((gpio_num_t) LED, 1);
	}


	for(;;){
		if(millis() - flashTime >= 500){
			if(!painted){
				audio->play({{ note, note, 500 }});
				for(int LED : LEDs){
					gpio_set_level((gpio_num_t) LED, 1);
				}
				for(int LED : RGBs){
					gpio_set_level((gpio_num_t) LED, 0);
				}
			}else{
				audio->stop();
				for(int LED : LEDs){
					gpio_set_level((gpio_num_t) LED, 0);
				}
				for(int LED : RGBs){
					gpio_set_level((gpio_num_t) LED, 1);
				}
			}

			for(int x = 0; x < canvas->width(); x++){
				for(int y = 0; y < canvas->height(); y++){
					const auto previousPixel = canvas->readPixel(x, y);
					if(!painted && previousPixel == TFT_BLACK){
						canvas->drawPixel(x, y, color);
					}else if(painted && previousPixel == color){
						canvas->drawPixel(x, y, TFT_BLACK);
					}
				}
			}

			delayMillis(10);
			painted = !painted;
			canvas->pushSprite(0, 0);
			flashTime = millis();
		}
	}
}

void JigHWTest::log(const char* property, const char* value){
	printf("%s:%s:%s\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, float value){
	printf("%s:%s:%f\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, double value){
	printf("%s:%s:%lf\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, bool value){
	printf("%s:%s:%s\n", currentTest, property, value ? "TRUE" : "FALSE");
}

void JigHWTest::log(const char* property, uint32_t value){
	printf("%s:%s:%lu\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, int32_t value){
	printf("%s:%s:%ld\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, const std::string& value){
	printf("%s:%s:%s\n", currentTest, property, value.c_str());
}

bool JigHWTest::BatteryCheck(){
	//Just in case BattVref was active
	const gpio_num_t RefSwitch = (gpio_num_t) Pins::get(Pin::BattVref);
	gpio_set_direction(RefSwitch, GPIO_MODE_OUTPUT);
	gpio_set_level(RefSwitch, 0);
	delayMillis(100);

	ADC adc(ADC_UNIT_1);

	const auto config = [&adc](int pin, adc_cali_handle_t& cali, std::unique_ptr<ADCReader>& reader){
		adc_unit_t unit;
		adc_channel_t chan;
		ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &chan));
		assert(unit == adc.getUnit());

		adc.config(chan, {
				.atten = ADC_ATTEN_DB_11,
				.bitwidth = ADC_BITWIDTH_12
		});

		const adc_cali_curve_fitting_config_t curveCfg = {
				.unit_id = unit,
				.chan = chan,
				.atten = ADC_ATTEN_DB_11,
				.bitwidth = ADC_BITWIDTH_12
		};
		ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curveCfg, &cali));

		static constexpr float Factor = 4.0f;
		static constexpr float Offset = 0;
		reader = std::make_unique<ADCReader>(adc, chan, cali, Offset, Factor);
	};

	adc_cali_handle_t cali;
	std::unique_ptr<ADCReader> reader;
	config(Pins::get(Pin::BattRead), cali, reader);

	constexpr uint16_t numReadings = 50;
	constexpr uint16_t readDelay = 10;
	uint32_t reading = 0;

	for(int i = 0; i < numReadings; i++){
		reading += reader->sample();
		vTaskDelay(readDelay / portTICK_PERIOD_MS);
	}
	reading /= numReadings;

	test->log("reading", reading);

	if(reading < BatVoltageMinimum){
		return false;
	}

	return true;
}

bool JigHWTest::VoltReferenceCheck(){
	const gpio_num_t RefSwitch = (gpio_num_t) Pins::get(Pin::BattVref);
	gpio_set_direction(RefSwitch, GPIO_MODE_OUTPUT);
	gpio_set_level(RefSwitch, 1);

	delayMillis(100);

	ADC adc(ADC_UNIT_1);

	const auto config = [&adc](int pin, adc_cali_handle_t& cali, std::unique_ptr<ADCReader>& reader){
		adc_unit_t unit;
		adc_channel_t chan;
		ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &chan));
		assert(unit == adc.getUnit());

		adc.config(chan, {
				.atten = ADC_ATTEN_DB_11,
				.bitwidth = ADC_BITWIDTH_12
		});

		const adc_cali_curve_fitting_config_t curveCfg = {
				.unit_id = unit,
				.chan = chan,
				.atten = ADC_ATTEN_DB_11,
				.bitwidth = ADC_BITWIDTH_12
		};
		ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curveCfg, &cali));

		static constexpr float Factor = 4.0f;
		static constexpr float Offset = 0;
		reader = std::make_unique<ADCReader>(adc, chan, cali, Offset, Factor);
	};

	adc_cali_handle_t cali;
	std::unique_ptr<ADCReader> reader;
	config(Pins::get(Pin::BattRead), cali, reader);

	constexpr uint16_t numReadings = 50;
	constexpr uint16_t readDelay = 10;
	uint32_t reading = 0;

	for(int i = 0; i < numReadings; i++){
		reading += reader->sample();
		vTaskDelay(readDelay / portTICK_PERIOD_MS);
	}
	reading /= numReadings;

	test->log("reading", reading);

	if(reading < VoltReference - VoltReferenceTolerance || reading > VoltReference + VoltReferenceTolerance){
		return false;
	}

	gpio_set_level(RefSwitch, 0);

	return true;
}

bool JigHWTest::SPIFFSTest(){
	auto ret = esp_vfs_spiffs_register(&spiffsConfig);
	if(ret != ESP_OK){
		test->log("spiffs", false);
		return false;
	}

	for(const auto& f : SPIFFSChecksums){
		auto file = fopen(f.name, "rb");
		if(file == nullptr){
			test->log("missing", f.name);
			return false;
		}

		uint32_t sum = calcChecksum(file);
		fclose(file);

		if(sum != f.sum){
			test->log("file", f.name);
			test->log("expected", (uint32_t) f.sum);
			test->log("got", (uint32_t) sum);

			return false;
		}
	}

	return true;
}

uint32_t JigHWTest::calcChecksum(FILE* file){
	if(file == nullptr) return 0;

#define READ_SIZE 512

	uint32_t sum = 0;
	uint8_t b[READ_SIZE];
	size_t read = 0;
	while((read = fread(b, 1, READ_SIZE, file))){
		for(int i = 0; i < read; i++){
			sum += b[i];
		}
	}

	return sum;
}

bool JigHWTest::RTCTest(){
	auto ret = i2c->write(0x51, { 0, 0, 0 }, 10);
	if(ret == ESP_OK){
		test->log("begin", true);
	}else{
		test->log("begin", false);
		return false;
	}

	return rtc->init();
}

bool JigHWTest::Time1(){
	auto t = rtc->getTime();
	auto unixt = mktime(&t);

	vTaskDelay(2000 / portTICK_PERIOD_MS);

	auto t2 = rtc->getTime();
	auto unixt2 = mktime(&t2);
	auto diff = std::difftime(unixt2, unixt);

	if(diff != 1 && diff != 2){
		test->log("time passage (expected 1s or 2s)", (uint32_t) diff);
		return false;
	}

	return true;
}

bool JigHWTest::Time2(){
	static constexpr size_t count = 1000;

	time_t lastTime = 0;

	for(uint32_t i = 0; i < count; i++){

		auto t = rtc->getTime();
		auto unixt = mktime(&t);

		if(i == 0){
			lastTime = unixt;
			continue;
		}
		vTaskDelay(1);

		auto diff = abs(difftime(unixt, lastTime));
		if(diff > 1){
			test->log("reading", i);
			test->log("diff", diff);
			JigHWTest::canvas->printf("diff: %.2f", diff);
			return false;
		}

		lastTime = unixt;
	}
	return true;
}

bool JigHWTest::IMUTest(){
	uint8_t data;
	auto ret = i2c->readReg(0x6A, 0x0FU, &data, 1, 10);
	return ret == ESP_OK && data == 0x6AU;
}

bool JigHWTest::IMUInterruptTest(){
	lsm6ds3tr_c_ctrl3_c_t ctrl3_c;

	auto ret = i2c->readReg(0x6A, LSM6DS3TR_C_CTRL3_C, (uint8_t*) &ctrl3_c, 1, 10);

	if(ret != 0){
		test->log("i2c read err", (int32_t) ret);
		return false;
	}

	ctrl3_c.sw_reset = 1;
	ret = i2c->writeReg(0x6A, LSM6DS3TR_C_CTRL3_C, (uint8_t*) &ctrl3_c, 1, 10);

	if(ret != 0){
		test->log("i2c write err", (int32_t) ret);
		return false;
	}
	delayMillis(50);

	gpio_set_direction((gpio_num_t) Pins::get(Pin::Imu_int1), GPIO_MODE_INPUT);
	gpio_set_direction((gpio_num_t) Pins::get(Pin::Imu_int2), GPIO_MODE_INPUT);

	if(gpio_get_level((gpio_num_t) Pins::get(Pin::Imu_int1)) || gpio_get_level((gpio_num_t) Pins::get(Pin::Imu_int2))){
		test->log("IMU interrupt 1", (bool) gpio_get_level((gpio_num_t) Pins::get(Pin::Imu_int1)));
		test->log("IMU interrupt 2", (bool) gpio_get_level((gpio_num_t) Pins::get(Pin::Imu_int2)));
		return false;
	}

	return true;
}

bool JigHWTest::buttons(){
	/** UNUSED */

	const auto cX = canvas->getCursorX();
	const auto cY = canvas->getCursorY();
	bool flash = false;
	uint32_t flashTime = 0;

	std::vector<bool> pressed(ButtonCount, false);
	std::vector<bool> released(ButtonCount, false);
	uint8_t pressCount = 0;
	uint8_t releaseCount = 0;
	for(;;){
		for(int i = 0; i < ButtonCount; i++){
			if(input->getState((Input::Button) i) && !pressed[i]){
				pressed[i] = true;
				pressCount++;
			}else if(!input->getState((Input::Button) i) && pressed[i] && !released[i]){
				released[i] = true;
				releaseCount++;
			}
		}

		if(pressCount == ButtonCount && releaseCount == ButtonCount) break;

		if(millis() - flashTime > 500){
			if(flash){
				canvas->fillRect(cX, cY - 4, 120, 8, TFT_BLACK);
			}else{
				canvas->setCursor(cX, cY);
				canvas->setTextColor(TFT_GOLD);
				canvas->printf("-PRESS BTNS-");
			}

			canvas->pushSprite(0, 0);
			flash = !flash;
			flashTime = millis();
		}

		canvas->fillRect(cX, cY + 6, 120, 8, TFT_BLACK);
		canvas->setTextColor(TFT_LIGHTGRAY);
		canvas->setCursor(cX - 3, cY + 10);
		canvas->printf("[");
		for(int i = 0; i < ButtonCount; i++){
			if(input->getState((Input::Button) i)){
				canvas->setTextColor(TFT_GOLD);
			}else if(pressed[i] && released[i]){
				canvas->setTextColor(TFT_BLUE);
			}else{
				canvas->setTextColor(TFT_DARKGRAY);
			}
			canvas->printf("-");
		}
		canvas->setTextColor(TFT_LIGHTGRAY);
		canvas->printf("]");
		canvas->pushSprite(0, 0);

		delayMillis(10);
	}

	canvas->fillRect(cX - 3, cY - 4, 120, 20, TFT_BLACK);
	canvas->setCursor(cX, cY);
	return pressCount == ButtonCount && releaseCount == ButtonCount;
}

bool JigHWTest::HWVersion(){
	uint16_t version = 0;
	bool result = EfuseMeta::readPID(version);

	if(!result){
		test->log("HW version", "couldn't PID read from efuse");
		return false;
	}

	if(version != 0){
		test->log("Existing HW version", (uint32_t) version);


		canvas->printf("PID:");
		canvas->setTextColor(TFT_GOLD);
		canvas->printf("%d ", version);
		canvas->setTextColor(TFT_WHITE);

		if(version == EfuseMeta::getHardcodedPID()){
			test->log("Already fused.", (uint32_t) version);
		}else{
			test->log("Wrong binary already fused!", (uint32_t) version);
			return false;
		}
	}

	uint8_t revision = 1;
	result = EfuseMeta::readRev(revision);

	if(!result){
		test->log("HW version", "couldn't revision read from efuse");
		return false;
	}

	if(revision != 0){
		test->log("Existing HW revision", (uint32_t) revision);


		canvas->printf("v");
		canvas->setTextColor(TFT_GOLD);
		canvas->printf("%d ", revision);
		canvas->setTextColor(TFT_WHITE);

		if(revision == EfuseMeta::getHardcodedRevision()){
			test->log("Already fused.", (uint32_t) revision);
			return true;
		}else{
			test->log("Wrong binary already fused!", (uint32_t) revision);
			return false;
		}
	}

	return EfuseMeta::write();
}
