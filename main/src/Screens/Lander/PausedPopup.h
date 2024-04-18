#ifndef ARTEMIS_FIRMWARE_PAUSEDPOPUP_H
#define ARTEMIS_FIRMWARE_PAUSEDPOPUP_H

#include <functional>
#include "LV_Interface/LVModal.h"
#include "LV_Interface/LVStyle.h"

class PausedPopup  : public LVModal {
public:
	PausedPopup(LVScreen* parent, const std::function<void()>& onResume, const std::function<void()>& onExit, bool releaseWait);

private:
	void buildStyles();
	void buildUI();

	std::function<void()> onResume;
	std::function<void()> onExit;

	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;

	lv_obj_t* gamePaused;
	lv_obj_t* yes;
	lv_obj_t* no;

	bool releaseWait = false;

	const lv_color_t Color = lv_color_make(255, 101, 0);
};

#endif //ARTEMIS_FIRMWARE_PAUSEDPOPUP_H
