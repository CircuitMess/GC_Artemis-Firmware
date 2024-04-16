#ifndef ARTEMIS_FIRMWARE_GAMEOVERPOPUP_H
#define ARTEMIS_FIRMWARE_GAMEOVERPOPUP_H

#include <functional>
#include "LV_Interface/LVModal.h"
#include "LV_Interface/LVStyle.h"

class GameOverPopup : public LVModal {
public:
	GameOverPopup(LVScreen* parent, std::function<void()> onYes, std::function<void()> onNo, uint32_t score, bool releaseWait);

private:
	void buildStyles();
	void buildUI();

	std::function<void()> onYes;
	std::function<void()> onNo;

	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;

	lv_obj_t* gameOver;
	lv_obj_t* yes;
	lv_obj_t* no;
	lv_obj_t* scoreLabel;
	lv_obj_t* playAgain;

	const uint32_t score;

	bool releaseWait = false;

	const lv_color_t Color = lv_color_make(255, 101, 0);
};


#endif //ARTEMIS_FIRMWARE_GAMEOVERPOPUP_H
