#ifndef CLOCKSTAR_FIRMWARE_DISPLAY_H
#define CLOCKSTAR_FIRMWARE_DISPLAY_H

#include <LovyanGFX.h>

class Display {
public:
	Display(uint8_t revision);
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

	const uint8_t revision;

};


#endif //CLOCKSTAR_FIRMWARE_DISPLAY_H
