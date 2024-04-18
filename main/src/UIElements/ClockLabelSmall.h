#ifndef CLOCKSTAR_FIRMWARE_CLOCKLABELSMALL_H
#define CLOCKSTAR_FIRMWARE_CLOCKLABELSMALL_H

#include "ClockLabel.h"

class ClockLabelSmall : public ClockLabel {
public:
	explicit ClockLabelSmall(lv_obj_t* parent);
	~ClockLabelSmall() override = default;

	virtual void updateVisuals() override;

private:
	void updateUI(const char* clockText, const char* ps) override;
	lv_obj_t* clock;
};


#endif //CLOCKSTAR_FIRMWARE_CLOCKLABELSMALL_H
