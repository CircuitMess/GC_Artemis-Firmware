#ifndef CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H
#define CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H

#include "ClockLabel.h"

class ClockLabelBig : public ClockLabel {
public:
	explicit ClockLabelBig(lv_obj_t* parent);
	~ClockLabelBig() override = default;
private:
	void updateUI(const char* clockText, const char* ps) override;


	static constexpr const char* getPath(char c);

	static constexpr uint8_t NumIcons = 5;
	lv_obj_t* icons[NumIcons]{};

	static constexpr const char* IconPaths[12] = {
			"S:/clockIcons/0.bin", "S:/clockIcons/1.bin", "S:/clockIcons/2.bin", "S:/clockIcons/3.bin", "S:/clockIcons/4.bin", "S:/clockIcons/5.bin",
			"S:/clockIcons/6.bin", "S:/clockIcons/7.bin", "S:/clockIcons/8.bin", "S:/clockIcons/9.bin", "S:/clockIcons/colon.bin", "S:/clockIcons/space.bin"
	};
};


#endif //CLOCKSTAR_FIRMWARE_CLOCKLABELBIG_H
