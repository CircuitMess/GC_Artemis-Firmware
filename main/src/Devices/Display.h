#ifndef CLOCKSTAR_FIRMWARE_DISPLAY_H
#define CLOCKSTAR_FIRMWARE_DISPLAY_H

#include <LovyanGFX.h>

class Display {
public:
	Display();
	virtual ~Display();

	LGFX_Device& getLGFX();

	void drawTest();

	void setRotation(bool rotation);

private:
	lgfx::Bus_SPI bus;
	lgfx::Panel_ST7735S panel;
	LGFX_Device lgfx;

	void setupBus();
	void setupPanel();

};


#endif //CLOCKSTAR_FIRMWARE_DISPLAY_H
