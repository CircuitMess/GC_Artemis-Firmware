#include "Display.h"
#include <Pins.hpp>
#include "Util/Services.h"
#include "Settings/Settings.h"

Display::Display(){
	setupBus();
	setupPanel();

	// LGFX init -> panel init -> bus init
	panel.setBus(&bus);
	lgfx.setPanel(&panel);
	lgfx.init();
}

Display::~Display(){
	bus.release();
}

void Display::setupBus(){
	lgfx::Bus_SPI::config_t cfg = {
		.freq_write = 40000000,
		.freq_read = 40000000,
		.pin_sclk = TFT_SCK,
		.pin_miso = -1,
		.pin_mosi = TFT_MOSI,
		.pin_dc = TFT_DC,
		.spi_mode = 0,
		.spi_3wire = false,
		.use_lock = false,
		.dma_channel = LGFX_ESP32_SPI_DMA_CH,
		.spi_host = SPI2_HOST
	};
	bus.config(cfg);
}

void Display::setupPanel(){
	auto& settings = *(Settings*) Services.get(Service::Settings);
	const uint8_t rotation = settings.get().screenRotate ? 3 : 1;

	lgfx::Panel_Device::config_t cfg = {
			.pin_cs = -1,
			.pin_rst = TFT_RST,
			.pin_busy = -1,
			.memory_width = 132,
			.memory_height = 132,
			.panel_width = 128,
			.panel_height = 128,
			.offset_x = 2,
			.offset_y = 1,
			.offset_rotation = rotation,
			.readable = false,
			.invert = false,
			.rgb_order = false,
			.dlen_16bit = false,
			.bus_shared = false
	};
	panel.config(cfg);
	panel.setBus(&bus);
}

LGFX_Device& Display::getLGFX(){
	return lgfx;
}

void Display::drawTest(){
	printf("Draw test...\n");
	lgfx.clear(TFT_RED);
	lgfx.setCursor(10, 10);
	lgfx.printf("Hellooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\n");
	lgfx.drawLine(0, 0, 127, 0, TFT_GREEN);
	lgfx.drawLine(0, 0, 0, 127, TFT_GREEN);
	lgfx.drawLine(0, 127, 127, 127, TFT_GREEN);
	lgfx.drawLine(127, 0, 127, 127, TFT_GREEN);
	printf("Done.\n");
}

void Display::setRotation(bool rotation){
	const uint8_t val = rotation ? 3 : 1;
	lgfx::Panel_Device::config_t cfg = lgfx.getPanel()->config();
	if(val == cfg.offset_rotation) return;

	cfg.offset_rotation = val;
	panel.config(cfg);
	lgfx.setPanel(&panel);
	lgfx.init();
}
