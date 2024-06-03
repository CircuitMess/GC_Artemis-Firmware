#ifndef ARTEMIS_FIRMWARE_LUNARLANDER_H
#define ARTEMIS_FIRMWARE_LUNARLANDER_H

#include "LV_Interface/LVScreen.h"
#include "vec2.hpp"
#include "Util/Events.h"
#include "mat3x3.hpp"
#include "LV_Interface/LVStyle.h"
#include <vector>

class LunarLander : public LVScreen {
public:
	LunarLander();
	~LunarLander() override;

private:
	virtual void onStop() override;

	std::vector<uint8_t> canvData;
	lv_obj_t* canvas;

	void buildTerrain();
	void drawTerrain();
	glm::vec2 movePoint(glm::vec2 point) const;
	void loop() override;

	std::vector<glm::vec2> terrainPoints;
	std::vector<std::pair<glm::vec2, glm::vec2>> terrainFlats;

	lv_obj_t* shuttle;

	uint64_t lastMillis;
	glm::vec2 pos = StartPos;
	glm::vec2 speed = StartSpeed;
	float angle = 0;
	float angleDir = 0;
	bool fire = false;
	lv_anim_t fireAnim;
	void startFireAnim();
	void stopFireAnim();
	float fuel = 100;

	uint64_t blinkTime = 0;

	uint32_t score = 0;
	static constexpr float calculateBonusMultiplier(float angle, float shuttlePlatformOffset, float speed,
													float leftoverFuel, float platformWidth, uint64_t elapsedTime);

	void setShuttlePos();

	glm::mat3 view;

	bool zoomed = false;
	float distToTerrain();

	void zoomStart();
	void zoomStop();

	void checkCollision();

	EventQueue evts;

	bool gameOver = false;
	bool paused = false;
	void resetLevel();
	void crashed();

	static const lv_color_t Color;

	lv_obj_t* fuelGauge;
	lv_obj_t* lbTime;
	lv_obj_t* lbScore;
	lv_obj_t* lbAlt;
	lv_obj_t* lbSpdHor;
	lv_obj_t* lbSpdVer;
	lv_obj_t* imgSpdHor;
	lv_obj_t* imgSpdVer;

	lv_obj_t* loadingText;

	LVStyle textStyle;

	uint64_t startTime;
	uint64_t endTime;

	void buildUI();
	void updateUI();

	class LVModal* modal = nullptr;

	bool exitFlag = false;

	static constexpr glm::vec2 StartPos = { 10, 36 };
	static constexpr glm::vec2 StartSpeed = { 3, 0 };

	static constexpr float LandingSpeedThreshold = 2.5f;
	static constexpr float LandingSpeedBonusThreshold = 0.5f; //under this speed bonus is guaranteed
	static constexpr float LandingAngleThreshold = 10.0f;

	static constexpr float MaxFlatWidth = 20.0f;

	static constexpr uint32_t LandingBaseReward = 50;
	static constexpr float PerfectLandingMultiplier = 3.0f;
	static constexpr uint64_t PerfectTimeBonusThreshold = 20000; //20s
	static constexpr uint64_t MinTimeBonusThreshold = 60000; //60s

	static constexpr uint32_t FireAnimationTime = 200; //[ms], time between fire images switching when playing animation

	static constexpr uint64_t BlinkInterval = 100;

};


#endif //ARTEMIS_FIRMWARE_LUNARLANDER_H
