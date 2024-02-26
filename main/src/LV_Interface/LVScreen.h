#ifndef BATCONTROLLER_FIRMWARE_LVSCREEN_H
#define BATCONTROLLER_FIRMWARE_LVSCREEN_H

#include "LVObject.h"
#include <unordered_set>
#include <functional>
#include <memory>

class LVGL;

class LVScreen : public LVObject {
public:
	LVScreen();
	virtual ~LVScreen();

	lv_group_t* getInputGroup() const;

	bool isRunning() const;

protected:
	lv_group_t* inputGroup;

	void transition(std::function<std::unique_ptr<LVScreen>()> create);

	LVGL* lvgl = nullptr;

private:
	void start(LVGL* lvgl);
	void stop();

	virtual void onStarting();
	virtual void onStart();
	virtual void onStop();

	bool running = false;

	friend LVGL;
	virtual void loop();

};


#endif //BATCONTROLLER_FIRMWARE_LVSCREEN_H
