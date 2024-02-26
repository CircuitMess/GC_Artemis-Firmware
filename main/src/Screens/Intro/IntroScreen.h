#ifndef ARTEMIS_FIRMWARE_INTROSCREEN_H
#define ARTEMIS_FIRMWARE_INTROSCREEN_H

#include "LV_Interface/LVScreen.h"

class IntroScreen : public LVScreen {
public:
	IntroScreen();
	virtual ~IntroScreen();
private:
	void onStart() override;
	void onStop() override;
	void loop() override;

	static constexpr const char* Cached[] = {
			"/intro/geek.bin", "/intro/cm.bin", "/intro/space.bin", "/intro/artemis.bin",
			"/intro/orangeBg.bin", "/intro/blackBg.bin"
	};

	static constexpr const char* ImagePaths[3] = {
			"S:/intro/geek.bin", "S:/intro/cm.bin", "S:/intro/space.bin"
	};

	static constexpr uint32_t ScrollTime = 500;
	static constexpr uint32_t WaitTime = 1000;

	lv_obj_t* bg;
	lv_obj_t* imageConts[3];
	lv_obj_t* images[3];

	enum {
		Scrolling, Waiting
	} state = Waiting;

	int8_t currentIndex = -1;
	uint32_t animTime = 0;
	lv_anim_t anim;

	lv_obj_t* phase2;
	void gotoPhase2();

	static void animFuncScroll(void *, int32_t);
	static void animFuncOpa(void *, int32_t);
};


#endif //ARTEMIS_FIRMWARE_INTROSCREEN_H
