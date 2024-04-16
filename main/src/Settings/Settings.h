#ifndef CLOCKSTAR_FIRMWARE_SETTINGS_H
#define CLOCKSTAR_FIRMWARE_SETTINGS_H

#include <nvs.h>
#include "core/lv_theme.h"
#include "Screens/Lock/Slider.h"

enum class Theme : uint8_t {
	Theme1 [[maybe_unused]],
	Theme2 [[maybe_unused]],
	Theme3 [[maybe_unused]],
	Theme4 [[maybe_unused]],
	Theme5 [[maybe_unused]],
	Theme6 [[maybe_unused]],
	Theme7 [[maybe_unused]],
	Theme8 [[maybe_unused]],
	Theme9 [[maybe_unused]],
	Default [[maybe_unused]] = Theme1
};

struct ThemeStruct {
	Theme theme = Theme::Default;
	lv_color_t primaryColor = lv_color_black();
	uint8_t opacity = LV_OPA_100;
	lv_color_t secondaryColor = lv_color_black();
	lv_color_t highlightColor = lv_color_black();
	lv_color_t backgroundColor = lv_color_black();
	lv_color_t dateColor = lv_color_white();
	SliderConfig sliderConfig = {};
	lv_coord_t sliderY = 0;
	lv_coord_t clockX = 0;
	lv_coord_t clockY = 0;
	lv_coord_t batteryX = 0;
	lv_coord_t batteryY = 0;
	lv_coord_t phoneX = 0;
	lv_coord_t phoneY = 0;
	lv_coord_t dateX = 0;
	lv_coord_t dateY = 0;
	bool verticalClock = false;
	int16_t clockPadding = 0;
	bool specialPhone = false;

	struct {
		lv_coord_t x = 0;
		lv_coord_t y = 0;
		lv_coord_t w = 128;
		lv_coord_t h = 11;
		lv_align_t align = LV_ALIGN_CENTER;
		lv_coord_t gapPad = 2;
		lv_flex_flow_t flex = LV_FLEX_FLOW_ROW;
		lv_flex_align_t mainAlign = LV_FLEX_ALIGN_CENTER;
		lv_flex_align_t crossAlign = LV_FLEX_ALIGN_CENTER;
		lv_flex_align_t trackAlign = LV_FLEX_ALIGN_CENTER;
		size_t maxNotifs = 20;
	} notifData;
};

inline static ThemeStruct createTheme(Theme theme){
	ThemeStruct data = {};
	data.theme = theme;

	switch(theme){
		case Theme::Theme1:{
			data.primaryColor = lv_color_make(255, 101, 0);
			data.secondaryColor = lv_color_make(0, 0, 0);
			data.highlightColor = lv_color_make(255, 101, 0);
			data.dateColor = lv_color_make(255, 101, 0);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 6;
			data.sliderConfig.end = 122;
			data.sliderConfig.y = 3;
			data.sliderY = 111;
			data.clockY = -14;
			data.batteryX = 4;
			data.batteryY = 4;
			data.phoneX = 56;
			data.phoneY = -57;
			data.notifData.y = 32;
			data.notifData.gapPad = 3;
			data.dateY = 14;
			data.notifData.maxNotifs = 10;

			break;
		}
		case Theme::Theme2:{
			data.primaryColor = lv_color_make(255, 38, 116);
			data.secondaryColor = lv_color_make(0, 0, 0);
			data.highlightColor = lv_color_make(255, 38, 116);
			data.dateColor = lv_color_make(255, 209, 0);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 108;
			data.sliderConfig.end = 20;
			data.sliderConfig.y = 0;
			data.sliderConfig.neverHide = true;
			data.sliderY = 102;
			data.clockY = -20;
			data.batteryX = 22;
			data.batteryY = 10;
			data.phoneX = 42;
			data.phoneY = -48;
			data.notifData.y = 17;
			data.specialPhone = true;
			data.dateY = 2;
			data.notifData.maxNotifs = 6;

			break;
		}
		case Theme::Theme3:{
			data.primaryColor = lv_color_make(255, 255, 255);
			data.secondaryColor = lv_color_make(0, 0, 0);
			data.highlightColor = lv_color_make(255, 255, 255);
			data.dateColor = lv_color_make(255, 255, 255);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 6;
			data.sliderConfig.end = 122;
			data.sliderConfig.y = 0;
			data.sliderY = 106;
			data.clockY = -18;
			data.batteryX = 1;
			data.batteryY = 1;
			data.phoneX = 44;
			data.phoneY = -60;
			data.specialPhone = true;
			data.notifData.y = 14;
			data.dateY = -40;
			data.notifData.maxNotifs = 9;

			break;
		}
		case Theme::Theme4:{
			data.primaryColor = lv_color_make(38, 43, 68);
			data.secondaryColor = lv_color_make(192, 203, 220);
			data.highlightColor = lv_color_make(38, 43, 68);
			data.backgroundColor = lv_color_make(192, 203, 220);
			data.dateColor = lv_color_make(38, 43, 68);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 122;
			data.sliderConfig.end = 8;
			data.sliderConfig.y = 4;
			data.sliderConfig.neverHide = true;
			data.sliderY = 109;
			data.clockX = -24;
			data.clockY = -8;
			data.verticalClock = true;
			data.clockPadding = 1;
			data.batteryX = 85;
			data.batteryY = 20;
			data.phoneX = 25;
			data.phoneY = -21;
			data.specialPhone = true;
			data.notifData.x = 85;
			data.notifData.y = 58;
			data.notifData.w = 42;
			data.notifData.h = 41;
			data.notifData.gapPad = 2;
			data.notifData.mainAlign = LV_FLEX_ALIGN_START;
			data.notifData.trackAlign = LV_FLEX_ALIGN_START;
			data.notifData.crossAlign = LV_FLEX_ALIGN_START;
			data.notifData.flex = LV_FLEX_FLOW_ROW_WRAP;
			data.notifData.align = LV_ALIGN_TOP_LEFT;
			data.dateY = 42;
			data.dateX = -22;
			data.notifData.maxNotifs = 14;

			break;
		}
		case Theme::Theme5:{
			data.primaryColor = lv_color_make(244, 180, 27);
			data.secondaryColor = lv_color_make(244, 180, 27);
			data.highlightColor = lv_color_make(244, 180, 27);
			data.backgroundColor = lv_color_make(55, 66, 99);
			data.dateColor = lv_color_make(244, 180, 27);
			data.opacity = LV_OPA_0;
			data.sliderConfig.start = 122;
			data.sliderConfig.end = 24;
			data.sliderConfig.y = 3;
			data.sliderConfig.neverHide = true;
			data.sliderY = 113;
			data.clockX = -28;
			data.clockY = -16;
			data.clockPadding = 4;
			data.verticalClock = true;
			data.batteryX = 118;
			data.batteryY = 20;
			data.phoneX = 40;
			data.phoneY = 0;
			data.specialPhone = true;
			data.notifData.x = 76;
			data.notifData.y = 6;
			data.notifData.w = 15;
			data.notifData.h = 94;
			data.notifData.gapPad = 8;
			data.notifData.mainAlign = LV_FLEX_ALIGN_CENTER;
			data.notifData.trackAlign = LV_FLEX_ALIGN_START;
			data.notifData.flex = LV_FLEX_FLOW_ROW_WRAP;
			data.notifData.align = LV_ALIGN_TOP_LEFT;
			data.dateY = 28;
			data.dateX = -28;
			data.notifData.maxNotifs = 5;

			break;
		}
		case Theme::Theme6:{
			data.primaryColor = lv_color_make(0, 0, 255);
			data.secondaryColor = lv_color_make(255, 255, 0);
			data.highlightColor = lv_color_make(255, 255, 255);
			data.dateColor = lv_color_make(0, 255, 255);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 126;
			data.sliderConfig.end = 2;
			data.sliderConfig.y = 0;
			data.sliderConfig.neverHide = true;
			data.sliderY = 110;
			data.clockY = -26;
			data.batteryX = 1;
			data.batteryY = 2;
			data.phoneX = 46;
			data.phoneY = -59;
			data.notifData.y = 62;
			data.notifData.x = 4;
			data.notifData.w = 128;
			data.notifData.h = 38;
			data.notifData.gapPad = 3;
			data.notifData.mainAlign = LV_FLEX_ALIGN_START;
			data.notifData.trackAlign = LV_FLEX_ALIGN_START;
			data.notifData.flex = LV_FLEX_FLOW_ROW_WRAP;
			data.notifData.align = LV_ALIGN_TOP_LEFT;
			data.specialPhone = true;
			data.dateY = -46;
			data.notifData.maxNotifs = 10;

			break;
		}
		case Theme::Theme7:{
			data.primaryColor = lv_color_make(251, 247, 54);
			data.secondaryColor = lv_color_make(251, 242, 54);
			data.highlightColor = lv_color_make(251, 247, 54);
			data.dateColor = lv_color_make(251, 242, 54);
			data.opacity = LV_OPA_0;
			data.sliderConfig.start = 122;
			data.sliderConfig.end = 20;
			data.sliderConfig.y = 3;
			data.sliderConfig.neverHide = true;
			data.sliderY = 111;
			data.clockY = -8;
			data.batteryX = 5;
			data.batteryY = 3;
			data.phoneX = 56;
			data.phoneY = -57;
			data.specialPhone = true;
			data.notifData.y = 27;
			data.notifData.x = 3;
			data.notifData.h = 15;
			data.dateY = 10;
			data.notifData.maxNotifs = 7;

			break;
		}
		case Theme::Theme8:{
			data.primaryColor = lv_color_make(94, 193, 0);
			data.secondaryColor = lv_color_make(0, 0, 0);
			data.highlightColor = lv_color_make(94, 193, 0);
			data.dateColor = lv_color_make(94, 193, 0);
			data.opacity = LV_OPA_100;
			data.sliderConfig.start = 122;
			data.sliderConfig.end = 6;
			data.sliderConfig.y = 3;
			data.sliderConfig.neverHide = true;
			data.sliderY = 111;
			data.clockY = -23;
			data.batteryX = 8;
			data.batteryY = 6;
			data.phoneX = 50;
			data.phoneY = -55;
			data.specialPhone = true;
			data.notifData.y = 28;
			data.notifData.h = 15;
			data.dateY = 9;
			data.dateX = 18;
			data.notifData.maxNotifs = 7;

			break;
		}
		case Theme::Theme9:{
			data.primaryColor = lv_color_make(255, 255, 0);
			data.secondaryColor = lv_color_make(255, 255, 255);
			data.highlightColor = lv_color_make(255, 210, 0);
			data.dateColor = lv_color_make(255, 65, 0);
			data.opacity = LV_OPA_0;
			data.sliderConfig.start = 118;
			data.sliderConfig.end = 8;
			data.sliderConfig.y = 3;
			data.sliderConfig.neverHide = true;
			data.sliderY = 109;
			data.clockY = -18;
			data.batteryX = 6;
			data.batteryY = 4;
			data.phoneX = 56;
			data.phoneY = -56;
			data.notifData.y = 24;
			data.specialPhone = true;
			data.dateY = 4;
			data.notifData.maxNotifs = 8;

			break;
		}
		default:{
			break;
		}
	}

	return data;
}

const static inline ThemeStruct Theme1 = createTheme(Theme::Theme1);
const static inline ThemeStruct Theme2 = createTheme(Theme::Theme2);
const static inline ThemeStruct Theme3 = createTheme(Theme::Theme3);
const static inline ThemeStruct Theme4 = createTheme(Theme::Theme4);
const static inline ThemeStruct Theme5 = createTheme(Theme::Theme5);
const static inline ThemeStruct Theme6 = createTheme(Theme::Theme6);
const static inline ThemeStruct Theme7 = createTheme(Theme::Theme7);
const static inline ThemeStruct Theme8 = createTheme(Theme::Theme8);
const static inline ThemeStruct Theme9 = createTheme(Theme::Theme9);
const static inline ThemeStruct Default = Theme1;

enum class DateFormat : uint8_t {
	Regular,
	Reverse
};

struct SettingsStruct {
	bool notificationSounds = true;
	uint8_t screenBrightness = 100;
	uint8_t sleepTime = 1;
	bool ledEnable = true;
	bool motionDetection = true;
	bool screenRotate = false;
	bool timeFormat24h = true;
	DateFormat dateFormat = DateFormat::Regular;
	ThemeStruct themeData = Default;
};

class Settings {
public:
	Settings();

	SettingsStruct get();
	void set(SettingsStruct& settings);
	void store();

	static constexpr uint8_t SleepSteps = 5;
	static constexpr const uint32_t SleepSeconds[SleepSteps] = { 0, 30, 60, 2 * 60, 5 * 60 };
	static constexpr const char* SleepText[SleepSteps] = { "OFF", "30 sec", "1 min", "2 min", "5 min" };

private:
	nvs_handle_t handle{};
	SettingsStruct settingsStruct;

	static constexpr const char* NVSNamespace = "Artemis";
	static constexpr const char* BlobName = "Settings";

	void load();
};


#endif //CLOCKSTAR_FIRMWARE_SETTINGS_H
