#ifndef CLOCKSTAR_FIRMWARE_MENUITEMALT_H
#define CLOCKSTAR_FIRMWARE_MENUITEMALT_H

#include "MenuItem.h"
#include <string>

class MenuItemAlt : public MenuItem {
public:
	MenuItemAlt(lv_obj_t* parent, const char* imagePath, const char* labelText);
	void setAltParams(const char* imagePathAlt, const char* labelTextAlt);

private:
	const std::string imagePath;
	const std::string labelText;

	std::string imagePathAlt;
	std::string labelTextAlt;

	void onClick();
	void onDefocus() override;

	void setOrig();
	void setAlt();

	bool isAlt = false;
};


#endif //CLOCKSTAR_FIRMWARE_MENUITEMALT_H
