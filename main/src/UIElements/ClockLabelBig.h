#ifndef CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H
#define CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H

#include "ClockLabel.h"
#include "Filepaths.hpp"

class ClockLabelBig : public ClockLabel {
public:
	explicit ClockLabelBig(lv_obj_t* parent, bool vertical = false, int16_t verticalPad = 0);
	~ClockLabelBig() override = default;
private:
	void updateUI(const char* clockText, const char* ps) override;

	static const char* getPath(char c);

	lv_obj_t* hourIcons[2]{};
	lv_obj_t* minuteIcons[2]{};
	lv_obj_t* colonIcon = nullptr;
	lv_obj_t* hours;
	lv_obj_t* minutes;
	bool vertical;
};


#endif //CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H
