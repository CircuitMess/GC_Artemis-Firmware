#include "Phone.h"
#include "Util/Events.h"

Phone::Phone(BLE::Server* server, BLE::Client* client) : ancs(client), cTime(client), bangle(server){
	auto reg = [this](NotifSource* src){
		src->setOnConnect([this, src](){ onConnect(src); });
		src->setOnDisconnect([this, src](){ onDisconnect(src); });
		src->setOnNotifAdd([this](Notif notif){ onAdd(std::move(notif)); });
		src->setOnNotifModify([this](Notif notif){ onModify(std::move(notif)); });
		src->setOnNotifRemove([this](uint32_t id){ onRemove(id); });
	};

	reg(&ancs);
	reg(&bangle);
}

bool Phone::isConnected(){
	return current != nullptr;
}

Phone::PhoneType Phone::getPhoneType(){
	if(current == &ancs) return PhoneType::IPhone;
	else if(current == &bangle) return PhoneType::Android;
	else return PhoneType::None;
}

auto Phone::findNotif(uint32_t id){
	return std::find_if(notifs.begin(), notifs.end(), [id](const auto& notif){ return notif.uid == id; });
}

Notif Phone::getNotif(uint32_t uid){
	auto notif = findNotif(uid);
	if(notif == notifs.end()) return {};
	return *notif;
}

std::vector<Notif> Phone::getNotifs(){
	return std::vector<Notif>(notifs.cbegin(), notifs.cend());
}

uint32_t Phone::getNotifsCount() const{
	return notifs.size();
}

void Phone::doPos(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionPos(id);
}

void Phone::doNeg(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionNeg(id);
}

void Phone::onConnect(NotifSource* src){
	current = src;
	Events::post(Facility::Phone, Event { .action = Event::Connected, .data = { .phoneType = getPhoneType() } });

	if(!notifs.empty()){
		notifs.clear();
		Events::post(Facility::Phone, Event { .action = Event::Cleared, .data = { .phoneType = getPhoneType() } });
	}
}

void Phone::onDisconnect(NotifSource* src){
	if(current != src) return;
	Events::post(Facility::Phone, Event { .action = Event::Disconnected, .data = { .phoneType = getPhoneType() } });
	current = nullptr;

	if(!notifs.empty()){
		notifs.clear();
		Events::post(Facility::Phone, Event { .action = Event::Cleared, .data = { .phoneType = getPhoneType() } });
	}
}

void Phone::onAdd(Notif notif){
	if(notif.title.empty() && notif.message.empty()) return;

	if(findNotif(notif.uid) != notifs.end()){
		onModify(std::move(notif));
		return;
	}

	while(notifs.size() >= MaxNotifs){
		const auto notif = notifs.front();
		notifs.pop_front();
		Events::post(Facility::Phone, Event { .action = Event::Removed, .data = { .addChgRem = { .id = notif.uid } } });
	}

	notifs.push_back(notif); // TODO: send whole notification, otherwise (by using a mutex) all newly unlocked tasks will rush after getNotif, and promptly get locked again by the mutex
	Events::post(Facility::Phone, Event { .action = Event::Added, .data = { .addChgRem = { .id = notif.uid } } });
}

void Phone::onModify(Notif notif){
	auto saved = findNotif(notif.uid);
	if(saved == notifs.end()){
		onAdd(std::move(notif));
		return;
	}

	*saved = notif;
	Events::post(Facility::Phone, Event { .action = Event::Changed, .data = { .addChgRem = { .id = notif.uid } } });
}

void Phone::onRemove(uint32_t id){
	auto notif = findNotif(id);
	if(notif == notifs.end()) return;

	notifs.erase(notif);
	Events::post(Facility::Phone, Event { .action = Event::Removed, .data = { .addChgRem = { .id = id } } });
}

void Phone::findPhoneStart(){
	if(current != &bangle) return;
	bangle.findPhoneStart();
}

void Phone::findPhoneStop(){
	if(current != &bangle) return;
	bangle.findPhoneStop();
}
