#ifndef CLOCKSTAR_FIRMWARE_INPUTLVGL_H
#define CLOCKSTAR_FIRMWARE_INPUTLVGL_H

#include <lvgl.h>
#include <map>
#include "Util/Events.h"
#include "Util/Threaded.h"
#include "../Devices/Input.h"

class InputLVGL : private Threaded {
public:
	InputLVGL();
	~InputLVGL();

	void read(lv_indev_drv_t* drv, lv_indev_data_t* data);
	static InputLVGL* getInstance();

	[[nodiscard]] lv_indev_t* getIndev() const;

	void invertDirections(bool invert);

	bool getInvertDirections() const;

protected:
	void loop() override;

private:
	static const std::map<Input::Button, lv_key_t> keyMap;
	lv_indev_t* inputDevice;

	bool invertedDirections = false;

	Input::Button lastKey = Input::Alt;
	Input::Data::Action action = Input::Data::Release;

	EventQueue queue;
	static constexpr size_t QueueSize = 5;

	static InputLVGL* instance;
};


#endif //CLOCKSTAR_FIRMWARE_INPUTLVGL_H
