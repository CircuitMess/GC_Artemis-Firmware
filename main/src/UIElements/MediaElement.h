#ifndef ARTEMIS_FIRMWARE_MEDIAELEMENT_H
#define ARTEMIS_FIRMWARE_MEDIAELEMENT_H

#include "Notifs/Phone.h"
#include "Notifs/MediaInfo.h"
#include "LV_Interface/LVSelectable.h"
#include "LV_Interface/LVStyle.h"
#include "LV_Interface/LVGIF.h"
#include <functional>

class MediaElement : public LVSelectable {
public:
	MediaElement(lv_obj_t* parent);
	~MediaElement() override;

	void setInfo(const MediaInfo& info);
	void setState(MediaState state);

	void loop();

	void playPause();
	void prev();
	void next();

private:
	void onSelect();
	void onDeselect();

	Phone* phone;
	MediaState state = MediaState::Playing;

	uint32_t actTime = 0;
	static constexpr uint32_t ActCooldown = 500; // [ms]

	uint32_t timerTime = 0;
	std::function<void()> timerCb;
	static constexpr uint32_t TimerPeriod = 100; // [ms]
	void timer(std::function<void()> cb);

	LVStyle textStyle;

	lv_obj_t* title = nullptr;
	lv_obj_t* artist = nullptr;
	lv_obj_t* album = nullptr;

	lv_obj_t* btns = nullptr;
	lv_obj_t* btnPrev = nullptr;
	lv_obj_t* btnPlay = nullptr;
	lv_obj_t* btnNext = nullptr;

	LVGIF playing;

	void buildUI();

};


#endif //ARTEMIS_FIRMWARE_MEDIAELEMENT_H
