#include "TesterScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Settings/Settings.h"
#include "Theme/theme.h"
#include "Util/Services.h"
#include "Util/stdafx.h"
#include "Devices/Input.h"
#include "Services/SleepMan.h"
#include <cstdlib>

TesterScreen::TesterScreen() : phone(*((Phone*) Services.get(Service::Phone))), queue(8){
	regenerateRandom();
	buildUI();
}

TesterScreen::~TesterScreen(){
	Events::unlisten(&queue);
}

void TesterScreen::onStart(){
	if(SleepMan* sleep = (SleepMan*) Services.get(Service::Sleep)){
		sleep->enAutoSleep(false);
	}

	Events::listen(Facility::Input, &queue);
	updateStatus();
	updateLog();
	updateSelector();
}

void TesterScreen::onStop(){
	Events::unlisten(&queue);

	if(SleepMan* sleep = (SleepMan*) Services.get(Service::Sleep)){
		sleep->enAutoSleep(true);
	}
}

void TesterScreen::loop(){
	Event evt{};
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			Input::Data* data = (Input::Data*) evt.data;
			Settings* settings = (Settings*) Services.get(Service::Settings);
			const bool rotated = settings != nullptr && settings->get().screenRotate;

			if(data->btn == Input::Alt){
				if(data->action == Input::Data::Press){
					altHeld = true;
					altModified = false;
				}else if(altHeld){
					altHeld = false;
					if(!altModified && paramMode){
						paramMode = false;
						updateSelector();
					}else if(!altModified){
						free(evt.data);
						transition([](){ return std::make_unique<MainMenu>(); });
						return;
					}
				}
			}else if(blocked){
				free(evt.data);
				return;
			}else if(data->btn == Input::Select && data->action == Input::Data::Press){
				if(paramMode){
					send();
					paramMode = false;
					updateSelector();
				}else if(paramKind(index) != ParamKind::None){
					paramMode = true;
					updateSelector();
				}else{
					send();
				}
			}else if((data->btn == Input::Up || data->btn == Input::Down) && data->action == Input::Data::Press){
				const bool up = (data->btn == Input::Up) != rotated;
				if(altHeld){
					altModified = true;
					scrollLog(up);
				}else if(paramMode){
					cyclePreset(!up);
				}else if(up){
					prev();
				}else{
					next();
				}
			}
		}
		free(evt.data);
	}

	if(millis() - lastPoll < PollInterval){
		return;
	}
	lastPoll = millis();

	updateStatus();
	updateLog();
}

void TesterScreen::buildUI(){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_style_set_text_color(textStyle, lv_color_make(200, 200, 200));
	lv_style_set_text_font(textStyle, &devin);
	lv_style_set_anim_speed(textStyle, ScrollSpeed);

	lv_obj_set_size(*this, 128, 128);
	lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(*this, settings->get().themeData.backgroundColor, 0);
	lv_obj_set_style_pad_all(*this, 0, 0);

	content = lv_obj_create(*this);
	lv_obj_set_size(content, 128, 128);
	lv_obj_set_pos(content, 0, 0);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

	blockedLabel = lv_label_create(*this);
	lv_label_set_text(blockedLabel, "NO\nPHONE");
	lv_obj_add_style(blockedLabel, textStyle, 0);
	lv_obj_set_style_text_font(blockedLabel, &lv_font_unscii_16, 0);
	lv_obj_set_style_text_align(blockedLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_flag(blockedLabel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_center(blockedLabel);

	lv_obj_t* title = lv_label_create(content);
	lv_label_set_text(title, "TESTER");
	lv_obj_add_style(title, textStyle, 0);
	lv_obj_set_style_text_font(title, &lv_font_unscii_8, 0);
	lv_obj_set_pos(title, 2, 2);

	statusLabel = lv_label_create(content);
	lv_obj_add_style(statusLabel, textStyle, 0);
	lv_obj_set_width(statusLabel, 74);
	lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_RIGHT, 0);
	lv_label_set_text(statusLabel, "");
	lv_obj_set_pos(statusLabel, 52, 3);

	logHeader = lv_label_create(content);
	lv_obj_add_style(logHeader, textStyle, 0);
	lv_obj_set_width(logHeader, 124);
	lv_label_set_long_mode(logHeader, LV_LABEL_LONG_SCROLL);
	lv_label_set_text(logHeader, "LOG empty  <- rx  -> tx");
	applyScrollPause(logHeader);
	lv_obj_set_pos(logHeader, 2, 13);

	for(size_t i = 0; i < LogLines; i++){
		logLabels[i] = lv_label_create(content);
		lv_obj_add_style(logLabels[i], textStyle, 0);
		lv_obj_set_width(logLabels[i], 124);
		lv_label_set_long_mode(logLabels[i], LV_LABEL_LONG_SCROLL);
		lv_label_set_text(logLabels[i], "-");
		lv_obj_set_pos(logLabels[i], 2, 22 + i * 9);
	}

	lv_obj_t* separator = lv_obj_create(content);
	lv_obj_set_size(separator, 124, 1);
	lv_obj_set_pos(separator, 2, 68);
	lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(separator, settings->get().themeData.primaryColor, 0);

	cmdLabel = lv_label_create(content);
	lv_obj_add_style(cmdLabel, textStyle, 0);
	lv_obj_set_style_text_font(cmdLabel, &lv_font_unscii_8, 0);
	lv_obj_set_width(cmdLabel, 124);
	lv_label_set_long_mode(cmdLabel, LV_LABEL_LONG_SCROLL);
	lv_obj_set_style_text_align(cmdLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(cmdLabel, 2, 72);

	lineLabel = lv_label_create(content);
	lv_obj_add_style(lineLabel, textStyle, 0);
	lv_obj_set_width(lineLabel, 124);
	lv_label_set_long_mode(lineLabel, LV_LABEL_LONG_SCROLL);
	lv_obj_set_pos(lineLabel, 2, 83);

	paramLabel = lv_label_create(content);
	lv_obj_add_style(paramLabel, textStyle, 0);
	lv_obj_set_width(paramLabel, 124);
	lv_label_set_long_mode(paramLabel, LV_LABEL_LONG_SCROLL);
	lv_obj_set_pos(paramLabel, 2, 92);

	hintTop = lv_label_create(content);
	lv_obj_add_style(hintTop, textStyle, 0);
	lv_obj_set_width(hintTop, 124);
	lv_obj_set_style_text_align(hintTop, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(hintTop, 2, 104);

	hintBottom = lv_label_create(content);
	lv_obj_add_style(hintBottom, textStyle, 0);
	lv_obj_set_width(hintBottom, 124);
	lv_obj_set_style_text_align(hintBottom, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(hintBottom, 2, 113);
}

void TesterScreen::applyMode(){
	if(content == nullptr || blockedLabel == nullptr){
		return;
	}

	if(blocked){
		lv_obj_add_flag(content, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(blockedLabel, LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_clear_flag(content, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(blockedLabel, LV_OBJ_FLAG_HIDDEN);
	}
}

std::string TesterScreen::oneLine(const std::string& text){
	std::string out;
	out.reserve(text.size() + 4);
	for(const char c : text){
		if(c == '\n'){
			out += "\\n";
		}else if(c == '\r'){
			out += "\\r";
		}else if((unsigned char) c < 0x20){
			out += '?';
		}else{
			out += c;
		}
	}
	return out;
}

void TesterScreen::applyScrollPause(lv_obj_t* label){
	lv_anim_t* anim = lv_anim_get(label, nullptr);
	if(anim == nullptr){
		return;
	}
	anim->playback_delay = ScrollPause;
	anim->repeat_delay = ScrollPause;
}

void TesterScreen::updateSelector(){
	if(cmdLabel == nullptr || lineLabel == nullptr || paramLabel == nullptr || hintTop == nullptr || hintBottom == nullptr){
		return;
	}

	if(paramMode){
		lv_label_set_text(cmdLabel, Commands[index]);
		applyScrollPause(cmdLabel);
		lv_label_set_text(hintTop, "UP/DN change param");
		lv_label_set_text(hintBottom, "SEL send  ALT cancel");
	}else{
		lv_label_set_text_fmt(cmdLabel, "< %s >", Commands[index]);
		applyScrollPause(cmdLabel);
		if(paramKind(index) == ParamKind::None){
			lv_label_set_text(hintTop, "UP/DN pick  SEL send");
		}else{
			lv_label_set_text(hintTop, "UP/DN pick  SEL edit");
		}
		lv_label_set_text(hintBottom, "ALT+UP/DN log  ALT back");
	}

	const std::string line = buildLine(index);
	if(line.empty()){
		lv_label_set_text_fmt(lineLabel, "%u/%u: closes GATT connection", (unsigned int) (index + 1), (unsigned int) CommandCount);
		applyScrollPause(lineLabel);
	}else{
		lv_label_set_text_fmt(lineLabel, "%u/%u: %s", (unsigned int) (index + 1), (unsigned int) CommandCount, line.c_str());
		applyScrollPause(lineLabel);
	}

	if(paramMode){
		lv_label_set_text_fmt(paramLabel, "< %s >", paramDesc(index).c_str());
		applyScrollPause(paramLabel);
	}else{
		lv_label_set_text(paramLabel, paramDesc(index).c_str());
		applyScrollPause(paramLabel);
	}
}

void TesterScreen::updateStatus(){
	if(statusLabel == nullptr || blockedLabel == nullptr){
		return;
	}

	std::string status;
	const Phone::PhoneType type = phone.getPhoneType();
	if(type == Phone::PhoneType::Android){
		status = "android";
		const std::optional<uint8_t> battery = phone.getPhoneBattery();
		if(battery.has_value()){
			status += " " + std::to_string(battery.value()) + "%";
		}
	}else if(type == Phone::PhoneType::IPhone){
		status = "iphone";
	}else{
		status = "no phone";
	}

	if(status == statusCache){
		return;
	}
	statusCache = status;
	lv_label_set_text(statusLabel, status.c_str());

	if(type == Phone::PhoneType::IPhone){
		lv_label_set_text(blockedLabel, "ONLY\nANDROID");
	}else if(type == Phone::PhoneType::None){
		lv_label_set_text(blockedLabel, "NO\nPHONE");
	}

	const bool nowBlocked = type != Phone::PhoneType::Android;
	if(nowBlocked != blocked){
		blocked = nowBlocked;
		applyMode();
	}
}

void TesterScreen::updateLog(){
	const uint32_t seq = phone.getAndroid().getLogSeq();
	if(logSeqCache.has_value() && logSeqCache.value() == seq){
		return;
	}
	logSeqCache = seq;
	logCache = phone.getAndroid().getLog();
	renderLog();

	if(presets[index] == Preset::Latest){
		updateSelector();
	}
}

void TesterScreen::renderLog(){
	const size_t total = logCache.size();
	const size_t maxOffset = total > LogLines ? total - LogLines : 0;
	if(logOffset > maxOffset){
		logOffset = maxOffset;
	}

	const size_t end = total - logOffset;
	const size_t start = end > LogLines ? end - LogLines : 0;

	for(size_t i = 0; i < LogLines; i++){
		if(logLabels[i] == nullptr){
			continue;
		}

		const size_t idx = start + i;
		if(idx >= end){
			lv_label_set_text(logLabels[i], "-");
			continue;
		}

		const Android::LogEntry& entry = logCache[idx];
		const unsigned int secs = entry.deltaMs / 1000;
		const unsigned int hundredths = (entry.deltaMs % 1000) / 10;
		lv_label_set_text_fmt(logLabels[i], "+%u.%02us %s %s", secs, hundredths, entry.tx ? "->" : "<-", oneLine(entry.line).c_str());
		applyScrollPause(logLabels[i]);
	}

	if(logHeader == nullptr){
		return;
	}

	if(total == 0){
		lv_label_set_text(logHeader, "LOG empty  <- rx  -> tx");
	}else{
		lv_label_set_text_fmt(logHeader, "LOG %u-%u/%u  <- rx  -> tx", (unsigned int) (start + 1), (unsigned int) end, (unsigned int) total);
	}
	applyScrollPause(logHeader);
}

void TesterScreen::prev(){
	paramMode = false;
	if(index == 0){
		index = CommandCount - 1;
	}else{
		index--;
	}
	updateSelector();
}

void TesterScreen::next(){
	paramMode = false;
	index++;
	if(index >= CommandCount){
		index = 0;
	}
	updateSelector();
}

void TesterScreen::scrollLog(bool older){
	if(older){
		logOffset++;
	}else if(logOffset > 0){
		logOffset--;
	}
	renderLog();
}

void TesterScreen::cyclePreset(bool forward){
	if(paramKind(index) == ParamKind::None){
		return;
	}

	const uint8_t count = (uint8_t) Preset::COUNT;
	uint8_t current = (uint8_t) presets[index];
	if(forward){
		current = (current + 1) % count;
	}else{
		current = (current + count - 1) % count;
	}
	presets[index] = (Preset) current;

	if(presets[index] == Preset::Random){
		regenerateRandom();
	}

	updateSelector();
}

void TesterScreen::regenerateRandom(){
	randomValue = ((uint32_t) rand() << 16) ^ (uint32_t) rand();
}

void TesterScreen::send(){
	const std::string line = buildLine(index);
	if(line.empty()){
		phone.getAndroid().dropConnection();
		return;
	}

	phone.getAndroid().sendRaw(line);

	if(presets[index] == Preset::Random){
		regenerateRandom();
		updateSelector();
	}

	updateLog();
}

TesterScreen::ParamKind TesterScreen::paramKind(size_t cmdIndex){
	const std::string cmd = Commands[cmdIndex];
	if(cmd == "notifPos" || cmd == "notifNeg"){
		return ParamKind::NotifId;
	}else if(cmd == "callReject" || cmd == "callAnswer"){
		return ParamKind::CallId;
	}
	return ParamKind::None;
}

std::string TesterScreen::paramValue(size_t cmdIndex){
	const ParamKind kind = paramKind(cmdIndex);
	if(kind == ParamKind::None){
		return "";
	}

	const Preset preset = presets[cmdIndex];
	if(preset == Preset::Latest){
		const std::optional<uint32_t> id = kind == ParamKind::CallId ? phone.getAndroid().getLastCallId() : phone.getAndroid().getLastNotifId();
		if(!id.has_value()){
			return InvalidId;
		}
		return std::to_string(id.value());
	}else if(preset == Preset::Invalid){
		return InvalidId;
	}else if(preset == Preset::Zero){
		return "0";
	}else if(preset == Preset::Max){
		return "4294967295";
	}

	return std::to_string(randomValue);
}

std::string TesterScreen::paramDesc(size_t cmdIndex){
	const ParamKind kind = paramKind(cmdIndex);
	if(kind == ParamKind::None){
		return "no params";
	}

	const char* what = kind == ParamKind::CallId ? "call" : "notif";
	const Preset preset = presets[cmdIndex];
	if(preset == Preset::Latest){
		const std::optional<uint32_t> id = kind == ParamKind::CallId ? phone.getAndroid().getLastCallId() : phone.getAndroid().getLastNotifId();
		if(!id.has_value()){
			return std::string("ID: no ") + what + " received yet, sending -1";
		}
		return std::string("ID: last ") + what + " received";
	}else if(preset == Preset::Invalid){
		return "ID: -1, invalid on purpose";
	}else if(preset == Preset::Zero){
		return "ID: 0, edge value";
	}else if(preset == Preset::Max){
		return "ID: uint32 max, edge value";
	}

	return "ID: random, new one each send";
}

std::string TesterScreen::buildLine(size_t cmdIndex){
	const std::string cmd = Commands[cmdIndex];

	if(cmd == DropCommand){
		return "";
	}else if(cmd == "version"){
		return cmd + ";" + std::to_string(Android::ProtocolVersion) + ";" + Android::FirmwareVersion;
	}else if(paramKind(cmdIndex) != ParamKind::None){
		return cmd + ";" + paramValue(cmdIndex);
	}

	return cmd;
}