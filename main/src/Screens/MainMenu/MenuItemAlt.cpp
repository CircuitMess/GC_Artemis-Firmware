#include "MenuItemAlt.h"

MenuItemAlt::MenuItemAlt(lv_obj_t* parent, const char* imagePath, const char* labelText)
		: MenuItem(parent, imagePath, labelText), imagePath(imagePath), labelText(labelText){

	lv_obj_add_event_cb(*this, [](lv_event_t* e){
		auto item = static_cast<MenuItemAlt*>(e->user_data);
		item->onClick();
	}, LV_EVENT_CLICKED, this);

}

void MenuItemAlt::setAltParams(const char* imagePathAlt, const char* labelTextAlt){
	this->imagePathAlt = imagePathAlt;
	this->labelTextAlt = labelTextAlt;

	if(isAlt){
		setAlt();
		MenuItem::onFocus();
	}
}

void MenuItemAlt::onClick(){
	if(isAlt){
		setOrig();
	}else{
		setAlt();
	}

	MenuItem::onFocus();
}

void MenuItemAlt::onDefocus(){
	if(isAlt){
		setOrig();
	}

	MenuItem::onDefocus();
}

void MenuItemAlt::setOrig(){
	constructVis(imagePath.c_str(), labelText.c_str());
	isAlt = false;
}

void MenuItemAlt::setAlt(){
	constructVis(imagePathAlt.c_str(), labelTextAlt.c_str());
	isAlt = true;
}
