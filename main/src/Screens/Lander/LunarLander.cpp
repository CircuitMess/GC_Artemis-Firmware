#include "LunarLander.h"
#include "Util/stdafx.h"
#include "Devices/Input.h"
#include "LV_Interface/LVGIF.h"
#include "GameOverPopup.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Theme/theme.h"
#include "LV_Interface/FSLVGL.h"
#include "Util/Services.h"
#include "PausedPopup.h"
#include "Services/ChirpSystem.h"
#include "Util/Notes.h"
#include <cmath>
#include <gtx/rotate_vector.hpp>
#include <gtx/closest_point.hpp>
#include <optional>

const lv_color_t LunarLander::Color = lv_color_make(255, 101, 0);

LunarLander::LunarLander() : evts(6){
	FSLVGL::unloadCache();

	lv_obj_set_style_bg_color(*this, lv_color_black(), 0);

	canvas = lv_canvas_create(*this);
	lv_obj_set_pos(canvas, 0, 0);
	lv_obj_set_size(canvas, 128, 128);
	lv_obj_add_flag(canvas, LV_OBJ_FLAG_FLOATING);

	canvData.resize(LV_CANVAS_BUF_SIZE_TRUE_COLOR(128, 128));
	lv_canvas_set_buffer(canvas, canvData.data(), 128, 128, LV_IMG_CF_TRUE_COLOR);

	view = glm::identity<glm::mat3>();

	buildTerrain();
	drawTerrain();

	shuttle = lv_img_create(*this);
	extern const lv_img_dsc_t Shuttle_small;
	lv_img_set_src(shuttle, &Shuttle_small);
	lv_img_set_pivot(shuttle, 5, 4);
	lv_img_set_antialias(shuttle, true);

	setShuttlePos();

	buildUI();
	updateUI();

	Events::listen(Facility::Input, &evts);

	lastMillis = startTime = millis();
}

LunarLander::~LunarLander(){
	Events::unlisten(&evts);
}

void LunarLander::onStop(){
	lv_obj_clean(*this);

	loadingText = lv_label_create(*this);
	if(loadingText == nullptr){
		return;
	}

	lv_obj_set_align(loadingText, LV_ALIGN_CENTER);
	lv_label_set_text(loadingText, "Loading...");
	lv_obj_set_style_text_font(loadingText, &devin, 0);
	lv_obj_set_style_text_color(loadingText, LunarLander::Color, 0);

	lv_obj_invalidate(*this);
	vTaskDelay(LV_DISP_DEF_REFR_PERIOD);
	lv_timer_handler();

	if(Settings* settings = (Settings*) Services.get(Service::Settings)){
		FSLVGL::loadCache(settings->get().themeData.theme);
	}
}

void LunarLander::loop(){
	const auto now = millis();
	const float dt = (float) (now - lastMillis) / 1000.0f;
	lastMillis = now;

	Event evt{};
	if(evts.get(evt, 0)){

		if(modal == nullptr){

			auto data = (Input::Data*) evt.data;

			if(data->action == Input::Data::Press && data->btn == Input::Up){
				angleDir = -1;
			}else if(data->action == Input::Data::Press && data->btn == Input::Down){
				angleDir = 1;
			}else if(data->action == Input::Data::Release && (data->btn == Input::Up || data->btn == Input::Down)){
				angleDir = 0;
			}else if(data->btn == Input::Select){
				fire = data->action == Input::Data::Press;
				if(fire){
					startFireAnim();
				}else{
					stopFireAnim();
				}
			}else if(data->btn == Input::Alt && data->action == Input::Data::Press){
				modal = new PausedPopup(this, [this](){
					paused = false;
					modal = nullptr;
				}, [this](){
					exitFlag = true;
					modal = nullptr;
				}, fire);

				endTime = millis();
				paused = true;
			}
		}

		free(evt.data);
	}

	if(exitFlag){
		delete modal;
		modal = nullptr;
		transition([](){ return std::make_unique<MainMenu>(); });
		return;
	}

	if(gameOver || paused){
		updateUI();
		return;
	}

	if(fuel <= 0){
		fire = false;
		stopFireAnim();
	}

	if(fire){
		const auto fireDir = glm::rotate(glm::vec2{ 0, -1 }, (float) M_PI * angle / 180);
		speed += fireDir * dt * 5.0f;
		fuel = std::clamp(fuel - 15.0f * dt, 0.0f, 100.0f);
	}

	pos += speed * dt;
	setShuttlePos();

	angle += angleDir * dt * 90.0f;
	lv_img_set_angle(shuttle, round(angle * 10.0f));

	const glm::vec2 pos(this->pos + glm::vec2(5, 4.5));
	if(pos.y <= -20.0f || pos.x <= -20.0f || pos.x >= 148.0f){
		speed = {};
		endTime = now;
		stopFireAnim();
		fire = false;
		crashed();
	}

	const auto toTerrain = distToTerrain();

	bool shouldZoom = toTerrain < 20;
	if(shouldZoom && !zoomed){
		zoomed = true;
		zoomStart();
	}else if(!shouldZoom && zoomed){
		zoomed = false;
		zoomStop();
	}else if(zoomed){
		const glm::vec2 screenPos(lv_obj_get_x(shuttle), lv_obj_get_y(shuttle));
		if((screenPos.x < 10 || screenPos.x + 9 > 118 || screenPos.y < 10 || screenPos.y + 9 > 118)){
			zoomStart();
		}
	}

	if(toTerrain <= 5){
		speed = {};
		endTime = now;
		stopFireAnim();
		checkCollision();
		fire = false;
	}

	if(fire){
		if(Settings* settings = (Settings*) Services.get(Service::Settings)){
			if(settings->get().notificationSounds){
				if(ChirpSystem* audio = (ChirpSystem*) Services.get(Service::Audio)){
					audio->play({ Chirp{ .startFreq = 750, .endFreq = 850, .duration = 6 } });
				}
			}
		}
	}

	updateUI();
}

void LunarLander::checkCollision(){
	const glm::vec2 pos(this->pos + glm::vec2(5, 4.5));

	std::optional<std::pair<glm::vec2, glm::vec2>> targetFlat;
	for(const auto& flat: terrainFlats){
		if(pos.x - 3 > flat.first.x && pos.x + 3 < flat.second.x){
			targetFlat = flat;
		}
	}

	if(!targetFlat){ // hit terrain, player loses
		crashed();
		return;
	}

	if(glm::length(speed) > LandingSpeedThreshold){ // too fast
		crashed();
		return;
	}

	if(angle > 180){
		do {
			angle -= 360;
		}while(angle > 180);
	}else if(angle < -180){
		do {
			angle += 360;
		}while(angle < -180);
	}

	if(abs(angle) > LandingAngleThreshold){ // not straight
		crashed();
		return;
	}

	gameOver = true;

	score += LandingBaseReward;
	const float targetFlatWidth = (targetFlat->second.x - targetFlat->first.x);
	const float targetFlatCenter = targetFlat->first.x + (targetFlatWidth / 2.0f);
	const float shuttlePlatformOffset = abs(pos.x - targetFlatCenter);
	const float multiplier = calculateBonusMultiplier(angle, shuttlePlatformOffset, glm::length(speed), fuel, targetFlatWidth, endTime - startTime);

	score += ceil(LandingBaseReward * multiplier * PerfectLandingMultiplier);

	score = std::min((uint32_t) 9999, score);

	if(Settings* settings = (Settings*) Services.get(Service::Settings)){
		if(settings->get().notificationSounds){
			if(ChirpSystem* audio = (ChirpSystem*) Services.get(Service::Audio)){
				audio->play({ Chirp{ .startFreq = NOTE_C4, .endFreq = NOTE_C4, .duration = 100 },
							  Chirp{ .startFreq = 0, .endFreq = 0, .duration = 100 },
							  Chirp{ .startFreq = NOTE_C4, .endFreq = NOTE_C4, .duration = 100 },
							  Chirp{ .startFreq = 0, .endFreq = 0, .duration = 100 },
							  Chirp{ .startFreq = NOTE_C4, .endFreq = NOTE_C4, .duration = 100 },
							  Chirp{ .startFreq = 0, .endFreq = 0, .duration = 100 },
							  Chirp{ .startFreq = NOTE_C4, .endFreq = NOTE_C4, .duration = 600 }
							});
			}
		}
	}

	vTaskDelay(2000);

	resetLevel();

}

void LunarLander::buildTerrain(){
	terrainPoints.clear();
	terrainFlats.clear();

	terrainPoints.emplace_back((int) 0, (int) 90 + rand() % 30);

	double dir = 0;
	for(float x = 0, y = terrainPoints.back().y, lastFlat = -1, i = 0; x < 128; i++){
		x += 2;

		double mainBias = -atan(((double) y - 64.0) / 16.0) / M_PI_2 + 0.7;

		double randVel = (double) (rand() % 1000 - 500) / 500.0;

		dir += randVel * 2.0 + mainBias * 3.0;
		y += round(dir);

		if(i < 2) continue;
		i = -1;

		if(abs(y - terrainPoints.back().y) < 2){
			y += 2;
		}

		if(y <= 80){
			terrainPoints.emplace_back(x, 80);
			y = 80;
			x += 10;
			dir = 1 + rand() % 4;
			lastFlat = x;
		}else if(y >= 127){
			terrainPoints.emplace_back(x, 127);
			y = 127;
			x += MaxFlatWidth;
			dir = -(1 + rand() % 4);
			lastFlat = x;
		}else if(dir <= 0.2 && (lastFlat == -1 || x - lastFlat > MaxFlatWidth)){
			terrainPoints.emplace_back(x, y);
			x += 10 * (rand() % 2 + 1);
			dir = (rand() % 10) - 5;
			if(abs(dir) <= 1){
				dir *= 2;
			}
			lastFlat = x;
		}

		if(lastFlat == x && x <= 115){
			terrainFlats.emplace_back(std::make_pair(terrainPoints.back(), glm::vec2{ x, y }));
		}

		terrainPoints.emplace_back(x, y);
	}
}

void LunarLander::drawTerrain(){
	const auto view = this->view;
	static const auto movePoint = [&view](glm::vec2 point){
		const glm::vec3 full(point, 1);
		glm::vec3 moved = view * full;
		moved = glm::round(moved / moved.z);
		return glm::vec2(moved);
	};

	std::vector<lv_point_t> terrain;
	terrain.reserve(terrainPoints.size());
	for(const auto& point: terrainPoints){
		const auto moved = movePoint(point);
		terrain.push_back(lv_point_t{ (lv_coord_t) moved.x, (lv_coord_t) moved.y });
	}

	std::vector<std::pair<lv_point_t, lv_point_t>> flats;
	flats.reserve(terrainFlats.size());
	for(const auto& flat: terrainFlats){
		const glm::vec2 firstMoved = movePoint(flat.first);
		const glm::vec2 secondMoved = movePoint(flat.second);

		flats.emplace_back(std::make_pair(lv_point_t{ (lv_coord_t) firstMoved.x, (lv_coord_t) firstMoved.y },
										  lv_point_t{ (lv_coord_t) secondMoved.x, (lv_coord_t) secondMoved.y }));
	}

	lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

	lv_draw_line_dsc_t draw;
	lv_draw_line_dsc_init(&draw);

	draw.color = Color;
	lv_canvas_draw_line(canvas, terrain.data(), terrain.size(), &draw);

	draw.color = lv_color_white();
	for(const auto& flat: flats){
		lv_point_t points[2] = { flat.first, flat.second };
		points[0].x++;
		lv_canvas_draw_line(canvas, points, 2, &draw);
	}
}

void LunarLander::setShuttlePos(){
	if(zoomed){
		glm::vec3 posFull(pos + glm::vec2(5, 4.5), 1);
		posFull = view * posFull;
		posFull /= posFull.z;
		posFull -= glm::vec3(8, 10.5, 0);
		lv_obj_set_pos(shuttle, round(posFull.x), round(posFull.y));
	}else{
		lv_obj_set_pos(shuttle, round(pos.x), round(pos.y));
	}
}

void LunarLander::zoomStart(){
	auto zoom = glm::identity<glm::mat3>();
	zoom[0][0] = zoom[1][1] = 2;

	auto moveCenter = glm::identity<glm::mat3>();
	moveCenter[2][0] = moveCenter[2][1] = -64.0f;

	auto moveCenter2 = glm::identity<glm::mat3>();
	moveCenter2[2][0] = moveCenter2[2][1] = 64.0f;

	auto move = glm::identity<glm::mat3>();
	move[2][0] = 64.0f - ((float) pos.x + 5.0f);
	move[2][1] = 64.0f - ((float) pos.y + 4.5f);

	view = moveCenter2 * zoom * moveCenter * move;
	drawTerrain();

	extern const lv_img_dsc_t Shuttle_big;
	lv_img_set_src(shuttle, &Shuttle_big);
	lv_img_set_pivot(shuttle, 9, 10);

	if(fire){
		stopFireAnim();
		startFireAnim();
	}

	setShuttlePos();
}

void LunarLander::zoomStop(){
	view = glm::identity<glm::mat3>();
	drawTerrain();

	extern const lv_img_dsc_t Shuttle_small;
	lv_img_set_src(shuttle, &Shuttle_small);
	lv_img_set_pivot(shuttle, 5, 4);

	if(fire){
		stopFireAnim();
		startFireAnim();
	}

	setShuttlePos();
}

float LunarLander::distToTerrain(){
	const glm::vec2 pos(this->pos + glm::vec2(5, 4.5));

	float min = -1;

	for(int i = 1; i < terrainPoints.size(); i++){
		const auto& p1 = terrainPoints[i - 1];
		const auto& p2 = terrainPoints[i];
		const auto closest = glm::closestPointOnLine(pos, p1, p2);
		const auto d = glm::distance(pos, closest);
		if(min == -1 || d < min){
			min = d;
		}
	}

	return min;
}

void LunarLander::buildUI(){
	extern const lv_font_t LanderFont;

	lv_style_set_text_font(textStyle, &LanderFont);
	lv_style_set_text_color(textStyle, Color);

	const auto addLabel = [this](const char* text, int x, int y){
		lv_obj_t* obj = lv_label_create(*this);
		lv_label_set_text(obj, text);
		lv_obj_add_style(obj, textStyle, 0);
		lv_obj_set_pos(obj, x, y);
		return obj;
	};

	addLabel("FUEL", 2, 2);
	addLabel("ALTITUDE", 2, 9);
	addLabel("SPEED", 9, 16);
	addLabel("SPEED", 9, 23);
	addLabel("TIME", 81, 2);
	addLabel("SCORE", 75, 9);

	lbAlt = addLabel("", 42, 9);
	lbSpdHor = addLabel("", 36, 16);
	lbSpdVer = addLabel("", 36, 23);
	lbTime = addLabel("", 101, 2);
	lbScore = addLabel("", 101, 9);

	const auto addArrow = [this](int x, int y){
		extern const lv_img_dsc_t Arrow;
		lv_obj_t* obj = lv_img_create(*this);
		lv_img_set_src(obj, &Arrow);
		lv_img_set_pivot(obj, 2, 2);
		lv_obj_set_pos(obj, x, y);
		return obj;
	};

	imgSpdHor = addArrow(2, 16);
	imgSpdVer = addArrow(2, 23);
	lv_img_set_angle(imgSpdVer, 900);

	fuelGauge = lv_slider_create(*this);
	lv_obj_set_pos(fuelGauge, 21, 2);
	lv_obj_set_size(fuelGauge, 56, 5); // height: 5
	lv_obj_set_style_border_width(fuelGauge, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(fuelGauge, Color, LV_PART_MAIN);
	lv_obj_set_style_pad_all(fuelGauge, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(fuelGauge, Color, LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(fuelGauge, LV_OPA_COVER, LV_PART_INDICATOR);

	lv_slider_set_range(fuelGauge, 0, 100);
	lv_slider_set_value(fuelGauge, 100, LV_ANIM_OFF);
}

void LunarLander::updateUI(){
	char text[99];

	const auto now = millis();
	const auto diff = (gameOver || paused) ? (endTime - startTime) : (now - startTime);

	auto mins = diff / (60 * 1000);
	const auto secs = diff / 1000 - mins * 60;
	if(mins > 99){
		mins = 99;
	}
	sprintf(text, "%02llu:%02llu", mins, secs);
	lv_label_set_text(lbTime, text);

	sprintf(text, "%04lu", score);
	lv_label_set_text(lbScore, text);

	for(int i = 1; i < terrainPoints.size(); i++){
		if(pos.x + 4.5 >= terrainPoints[i - 1].x && pos.x + 4.5 <= terrainPoints[i].x){
			const float y = (terrainPoints[i - 1].y + terrainPoints[i].y) / 2;
			const int diff = round(std::clamp(y - (pos.y + 10), 0.0f, 130.0f) * 4.0f);
			sprintf(text, "%d", diff);
			lv_label_set_text(lbAlt, text);
		}
	}

	lv_slider_set_value(fuelGauge, (int) round(fuel), LV_ANIM_OFF);

	sprintf(text, "%d", (int) round(abs(speed.x * 4.0f)));
	lv_label_set_text(lbSpdHor, text);

	sprintf(text, "%d", (int) round(abs(speed.y * 4.0f)));
	lv_label_set_text(lbSpdVer, text);

	if(speed.x >= 0){
		lv_img_set_angle(imgSpdHor, 0);
	}else{
		lv_img_set_angle(imgSpdHor, 1800);
	}

	if(speed.y >= 0){
		lv_img_set_angle(imgSpdVer, 900);
	}else{
		lv_img_set_angle(imgSpdVer, -900);
	}
}

void LunarLander::startFireAnim(){
	static lv_anim_exec_xcb_t smallCB = [](void* var, int32_t value){
		lv_obj_t* obj = (lv_obj_t*) var;
		if(value){
			extern const lv_img_dsc_t Shuttle_small_fire1;
			lv_img_set_src(obj, &Shuttle_small_fire1);
		}else{
			extern const lv_img_dsc_t Shuttle_small_fire2;
			lv_img_set_src(obj, &Shuttle_small_fire2);
		}
	};

	static lv_anim_exec_xcb_t bigCB = [](void* var, int32_t value){
		lv_obj_t* obj = (lv_obj_t*) var;
		if(value){
			extern const lv_img_dsc_t Shuttle_big_fire1;
			lv_img_set_src(obj, &Shuttle_big_fire1);
		}else{
			extern const lv_img_dsc_t Shuttle_big_fire2;
			lv_img_set_src(obj, &Shuttle_big_fire2);
		}
	};

	lv_anim_init(&fireAnim);
	lv_anim_set_exec_cb(&fireAnim, zoomed ? bigCB : smallCB);
	lv_anim_set_values(&fireAnim, 0, 1);
	lv_anim_set_time(&fireAnim, FireAnimationTime);
	lv_anim_set_playback_time(&fireAnim, FireAnimationTime);
	lv_anim_set_path_cb(&fireAnim, lv_anim_path_step);
	lv_anim_set_repeat_count(&fireAnim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_var(&fireAnim, shuttle);
	lv_anim_start(&fireAnim);

}

void LunarLander::stopFireAnim(){
	lv_anim_del(shuttle, nullptr);
	if(zoomed){
		extern const lv_img_dsc_t Shuttle_big;
		lv_img_set_src(shuttle, &Shuttle_big);
	}else{
		extern const lv_img_dsc_t Shuttle_small;
		lv_img_set_src(shuttle, &Shuttle_small);
	}
}

constexpr float LunarLander::calculateBonusMultiplier(float angle, float shuttlePlatformOffset, float speed,
													  float leftoverFuel, float platformWidth, uint64_t timeElapsed){
	const float angleMatch = 1.0f - (abs(angle) / LandingAngleThreshold);
	const float offsetMatch = 1.0f - shuttlePlatformOffset / (MaxFlatWidth / 2.0f);
	const float speedMatch = speed < LandingSpeedBonusThreshold ? 1.0f : (1.0f - (speed / LandingSpeedThreshold));
	const float fuelMatch = sqrt(leftoverFuel / 100.0f); //fuel should be used, so sqrt() to not penalize some usage
	const float platformWidthMatch = 1.0f - (platformWidth / MaxFlatWidth);
	const float timeMatch = timeElapsed < PerfectTimeBonusThreshold ? 1.0f : (timeElapsed > MinTimeBonusThreshold ? 0.0f :
																			  1.0f - ((float) (timeElapsed - PerfectTimeBonusThreshold) /
																					  (MinTimeBonusThreshold - PerfectTimeBonusThreshold)));

	return (angleMatch + offsetMatch + speedMatch + fuelMatch + platformWidthMatch + timeMatch) / 6.0f;
}

void LunarLander::resetLevel(){
	pos = StartPos;
	speed = StartSpeed;
	angle = 0;
	angleDir = 0;
	fire = false;
	fuel = 100;

	view = glm::identity<glm::mat3>();
	zoomed = false;
	gameOver = false;

	buildTerrain();
	drawTerrain();
	setShuttlePos();
	updateUI();

	stopFireAnim();

	lastMillis = startTime = millis();
}

void LunarLander::crashed(){
	lv_obj_del(shuttle);
	gameOver = true;

	delete modal;
	modal = new GameOverPopup(this, [this](){
		score = 0;

		shuttle = lv_img_create(*this);
		extern const lv_img_dsc_t Shuttle_small;
		lv_img_set_src(shuttle, &Shuttle_small);
		lv_img_set_pivot(shuttle, 5, 4);
		lv_img_set_antialias(shuttle, true);

		resetLevel();
		modal = nullptr;
	}, [this](){
		exitFlag = true;
		modal = nullptr;
	}, score, fire);
}
