#include "LVGL.h"
#include <lvgl.h>
#include "LVScreen.h"
#include "InputLVGL.h"
#include "Util/Services.h"
#include "Services/SleepMan.h"

LVGL::LVGL(Display& display) : Threaded("LVGL", 4 * 1024, 6, 1), display(display){
	lv_init();
	lv_disp_draw_buf_init(&lvDrawBuf, drawBuffer, nullptr, sizeof(drawBuffer) / 2);

	lv_disp_drv_init(&lvDispDrv);
	lvDispDrv.hor_res = 128;
	lvDispDrv.ver_res = 128;
	lvDispDrv.flush_cb = flush;
	lvDispDrv.draw_buf = &lvDrawBuf;
	lvDispDrv.user_data = this;
	lvDisplay = lv_disp_drv_register(&lvDispDrv);
}

LVGL::~LVGL(){
	stop();
}

void LVGL::flush(lv_disp_drv_t* dispDrv, const lv_area_t* area, lv_color_t* pixels){
	auto lvgl = static_cast<LVGL*>(dispDrv->user_data);
	auto lgfx = lvgl->display.getLGFX();

	auto x = area->x1;
	auto y = area->y1;
	auto w = (area->x2 - area->x1 + 1);
	auto h = (area->y2 - area->y1 + 1);
	auto data = &pixels->full;

	lgfx.pushImage(x, y, w, h, data);

	lv_disp_flush_ready(dispDrv);
}

void LVGL::loop(){
	auto sleep = (SleepMan*) Services.get(Service::Sleep);
	if(sleep){
		sleep->loop();
	}

	if(currentScreen){
		currentScreen->loop();
	}

	auto ttn = lv_timer_handler();
	if(ttn <= 0 || ttn > LV_DISP_DEF_REFR_PERIOD) ttn = 1;
	vTaskDelay(ttn);
}

void LVGL::startScreen(std::function<std::unique_ptr<LVScreen>()> create){
	stopScreen();

	lv_obj_t* tmp = lv_obj_create(nullptr);
	lv_scr_load_anim(tmp, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

	currentScreen.reset();

	currentScreen = create();
	currentScreen->start(this);
	lv_scr_load_anim(*currentScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

void LVGL::stopScreen(){
	if(!currentScreen) return;
	currentScreen->stop();
	lv_indev_set_group(InputLVGL::getInstance()->getIndev(), nullptr);
}

lv_disp_t* LVGL::disp() const{
	return lvDisplay;
}

void LVGL::rotateScreen(bool rotation){
	display.setRotation(rotation);
}
