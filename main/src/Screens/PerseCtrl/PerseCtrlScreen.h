#ifndef ARTEMIS_FIRMWARE_PERSECTRLSCREEN_H
#define ARTEMIS_FIRMWARE_PERSECTRLSCREEN_H

#include "LV_Interface/LVScreen.h"
#include "WiFiSTA.h"
#include "TCPClient.h"
#include "Feed.h"
#include "PairService.h"
#include "Comm.h"

class PerseCtrlScreen : public LVScreen {
public:
	PerseCtrlScreen();
	~PerseCtrlScreen() override;

	virtual void onStart() override;
	virtual void onStop() override;

private:
	WiFiSTA& wifi;
	std::unique_ptr<TCPClient> tcp;
	Feed feed;
	std::unique_ptr<Comm> comm;
	std::unique_ptr<PairService> pair;

	uint8_t* feedBuf;
	lv_img_dsc_t imgDsc = {
			.header = {
					.cf = LV_IMG_CF_TRUE_COLOR,
					.always_zero = 0, .reserved = 0,
					.w = 160, .h = 120
			}
	};
	lv_obj_t* feedImg;

	EventQueue evts;

	void loop() override;

	lv_obj_t* pairLabel;

	bool paired = false;
	bool cacheReturned = false;

	uint8_t camPos = 50;
	int8_t camDir = 0;

	uint64_t sendTime = 0;
	static constexpr uint32_t SendInterval = 50; // [ms]
	static lv_color_t Color;
};


#endif //ARTEMIS_FIRMWARE_PERSECTRLSCREEN_H
