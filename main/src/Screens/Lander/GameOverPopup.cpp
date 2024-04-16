#include "GameOverPopup.h"
#include "Screens/Settings/LabelElement.h"
#include "Theme/theme.h"

#include <utility>
#include <cstdio>

extern const lv_font_t LanderFont;

GameOverPopup::GameOverPopup(LVScreen* parent, std::function<void()> onYes, std::function<void()> onNo, uint32_t score, bool releaseWait) : LVModal(parent),
																																			score(score){
	this->onYes = std::move(onYes);
	this->onNo = std::move(onNo);
	this->releaseWait = releaseWait;
	buildStyles();
	buildUI();
}

void GameOverPopup::buildStyles(){
	lv_style_set_border_width(defaultStyle, 1);
	lv_style_set_border_opa(defaultStyle, 0);
	lv_style_set_pad_all(defaultStyle, 1);
	lv_style_set_bg_opa(defaultStyle, 0);

	lv_style_set_border_width(focusedStyle, 1);
	lv_style_set_border_color(focusedStyle, Color);
	lv_style_set_border_opa(focusedStyle, LV_OPA_COVER);

	lv_style_set_text_font(labelStyle, &LanderFont);
	lv_style_set_text_color(labelStyle, Color);
	lv_style_set_text_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_text_align(labelStyle, LV_TEXT_ALIGN_CENTER);
}

void GameOverPopup::buildUI(){
	auto parent = lv_obj_get_parent(*this);
	lv_obj_set_style_bg_img_src(parent, nullptr, LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(parent, lv_color_black(), LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(parent, 1, LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(parent, Color, LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(parent, LV_OPA_COVER, LV_STATE_DEFAULT);

	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);

	gameOver = lv_obj_create(*this);
	lv_obj_set_size(gameOver, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_align(gameOver, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(gameOver, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* game = lv_label_create(gameOver);
	lv_label_set_text(game, "GAME");
	lv_obj_set_style_text_color(game, Color, LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(game, &lv_font_unscii_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(game, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_opa(game, LV_OPA_COVER, LV_STATE_DEFAULT);

	lv_obj_t* over = lv_label_create(gameOver);
	lv_label_set_text(over, "OVER");
	lv_obj_set_style_text_color(over, Color, LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(over, &lv_font_unscii_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(over, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_opa(over, LV_OPA_COVER, LV_STATE_DEFAULT);

	//score, Play again?

	char txt[50];
	sprintf(txt, "YOUR SCORE: %04lu", score);
	scoreLabel = lv_label_create(*this);
	lv_label_set_text(scoreLabel, txt);
	lv_obj_set_style_text_color(scoreLabel, Color, LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(scoreLabel, &LanderFont, LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(scoreLabel, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_opa(scoreLabel, LV_OPA_COVER, LV_STATE_DEFAULT);


	playAgain = lv_label_create(*this);
	lv_label_set_text(playAgain, "PLAY AGAIN");
	lv_obj_set_style_text_color(playAgain, Color, LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(playAgain, &LanderFont, LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(playAgain, LV_OPA_COVER, LV_STATE_DEFAULT);
	lv_obj_set_style_opa(playAgain, LV_OPA_COVER, LV_STATE_DEFAULT);

	lv_obj_t* container = lv_obj_create(*this);
	lv_obj_set_size(container, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

	yes = lv_label_create(container);
	lv_obj_set_height(yes, 9);
	lv_label_set_text(yes, "YES");
	lv_obj_add_style(yes, labelStyle, LV_STATE_DEFAULT);
	lv_obj_add_style(yes, focusedStyle, LV_STATE_FOCUSED);
	lv_obj_add_style(yes, defaultStyle, LV_STATE_DEFAULT);
	lv_obj_add_event_cb(yes, [](lv_event_t* event){
		auto popup = static_cast<GameOverPopup*>(event->user_data);
		if(popup->releaseWait){
			popup->releaseWait = false;
			return;
		}
		if(popup->onYes) popup->onYes();
		delete popup;
	}, LV_EVENT_CLICKED, this);
	lv_group_add_obj(inputGroup, yes);
	lv_obj_add_flag(yes, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(yes, LV_OBJ_FLAG_SCROLLABLE);


	no = lv_label_create(container);
	lv_obj_set_height(no, 9);
	lv_label_set_text(no, "NO");
	lv_obj_add_style(no, labelStyle, LV_STATE_DEFAULT);
	lv_obj_add_style(no, focusedStyle, LV_STATE_FOCUSED);
	lv_obj_add_style(no, defaultStyle, LV_STATE_DEFAULT);
	lv_obj_add_event_cb(no, [](lv_event_t* event){
		auto popup = static_cast<GameOverPopup*>(event->user_data);
		if(popup->releaseWait){
			popup->releaseWait = false;
			return;
		}
		if(popup->onNo) popup->onNo();
		delete popup;
	}, LV_EVENT_CLICKED, this);
	lv_group_add_obj(inputGroup, no);
	lv_obj_add_flag(no, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(no, LV_OBJ_FLAG_SCROLLABLE);

}
