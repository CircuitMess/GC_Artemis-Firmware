#include "Pins.hpp"
#include "Util/EfuseMeta.h"

static const char* TAG = "Pins";

int Pins::get(Pin pin){
	if(instance == nullptr){
		instance = new Pins();
		setLatest(); // TODO undo this
		/*uint8_t revision = 0;
		EfuseMeta::readRev(revision);

		if(revision == 0 || revision == 1 || revision == 2){
			instance->currentMap = &instance->Revision1;
		}else if(revision == 3){
			instance->currentMap = &instance->Revision2;
		}else{
			while(true){
				EfuseMeta::log();
				while(true);
			}
		}*/
	}

	assert(instance != nullptr);

	PinMap* pinMap = instance->currentMap;

	if(pinMap == nullptr){
		ESP_LOGE(TAG, "Pin map is invalid.!\n");
		return -1;
	}

	if(!pinMap->contains(pin)){
		ESP_LOGE(TAG, "Pin %d not mapped!\n", (int)pin);
		return -1;
	}

	return pinMap->at(pin);
}

void Pins::setLatest(){
	if(instance == nullptr){
		instance = new Pins();
	}

	instance->currentMap = instance->pinMaps.back();
}

Pins::Pins(){
	initPinMaps();
}

void Pins::initPinMaps(){
	Revision1 = {
		{ Pin::BtnDown, 10 },
		{ Pin::BtnUp, 9 },
		{ Pin::BtnSelect, 8 },
		{ Pin::BtnAlt, 21 },
		{ Pin::LedBl, 37 },
		{ Pin::Buzz, 11 },
		{ Pin::BattRead, 6 },
		{ Pin::BattVref, -1 },
		{ Pin::Usb, 47 },
		{ Pin::I2cSda, 35 },
		{ Pin::I2cScl, 36 },
		{ Pin::TftSck, 41 },
		{ Pin::TftMosi, 40 },
		{ Pin::TftDc, 39 },
		{ Pin::TftRst, 38 },
		{ Pin::Rgb_r, 33 },
		{ Pin::Rgb_g, 34 },
		{ Pin::Rgb_b, 48 },
		{ Pin::ChrgIn, 1 },
		{ Pin::ChrgOut, 2 },
		{ Pin::Pwdn, 42 },
		{ Pin::Imu_int1, 4 },
		{ Pin::Imu_int2, 5 },
		{ Pin::Led_1, 46 },
		{ Pin::Led_2, 45 },
		{ Pin::Led_3, 44 },
		{ Pin::Led_4, 43 },
		{ Pin::Led_5, 18 },
		{ Pin::Led_6, 17 },
		{ Pin::JigStatus, -1 },
	};

	Revision2 = {
		{ Pin::BtnDown, 2 },
		{ Pin::BtnUp, 1 },
		{ Pin::BtnSelect, 3 },
		{ Pin::BtnAlt, 21 },
		{ Pin::LedBl, 33 },
		{ Pin::Buzz, 47 },
		{ Pin::BattRead, 5 },
		{ Pin::BattVref, 4 },
		{ Pin::Usb, 42 },
		{ Pin::I2cSda, 40 },
		{ Pin::I2cScl, 41 },
		{ Pin::TftSck, 36 },
		{ Pin::TftMosi, 35 },
		{ Pin::TftDc, 48 },
		{ Pin::TftRst, 34 },
		{ Pin::Rgb_r, 14 },
		{ Pin::Rgb_g, 13 },
		{ Pin::Rgb_b, 12 },
		{ Pin::ChrgIn, 7 },
		{ Pin::ChrgOut, 6 },
		{ Pin::Pwdn, 37 },
		{ Pin::Imu_int1, 38 },
		{ Pin::Imu_int2, 39 },
		{ Pin::Led_1, 46 },
		{ Pin::Led_2, 45 },
		{ Pin::Led_3, 44 },
		{ Pin::Led_4, 43 },
		{ Pin::Led_5, 18 },
		{ Pin::Led_6, 17 },
		{ Pin::JigStatus, -1 },
	};
}

