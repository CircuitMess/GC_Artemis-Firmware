#include "Phone.h"
#include "Util/Events.h"
#include <functional>
#include <esp_log.h>

Phone::Phone(BLE::Server* server, BLE::Client* client) : ancs(client), cTime(client), android(server){
	auto reg = [this](NotifSource* src){
		src->setOnConnect([this, src](){ onConnect(src); });
		src->setOnDisconnect([this, src](){ onDisconnect(src); });
		src->setOnNotifAdd([this](Notif notif){ onAdd(std::move(notif)); });
		src->setOnNotifModify([this](Notif notif){ onModify(std::move(notif)); });
		src->setOnNotifRemove([this](uint32_t id){ onRemove(id); });
	};

	reg(&ancs);
	reg(&android);

	// media registration (only Android currently implements MediaSource)
	auto mreg = [this](MediaSource* src){
		src->setOnConnect([this, src](){ onMediaConnect(src); });
		src->setOnDisconnect([this, src](){ onMediaDisconnect(src); });
		src->setOnMediaInfo([this](Media media){ onMediaInfo(std::move(media)); });
		src->setOnMediaState([this](MediaState state){ onMediaState(state); });
	};

	mreg(&android);
}

bool Phone::isConnected(){
	return current != nullptr;
}

Phone::PhoneType Phone::getPhoneType(){
	if(current == &ancs) return PhoneType::IPhone;
	else if(current == &android) return PhoneType::Android;
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

Media Phone::getMedia(){
	return *currentMedia;
}

MediaState Phone::getMediaState(){
	return currentMediaState;
}

void Phone::doPos(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionPos(id);
}

void Phone::doNeg(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionNeg(id);
}

void Phone::doMediaPlay(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPlay();
}

void Phone::doMediaPause(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPause();
}

void Phone::doMediaNext(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaNext();
}

void Phone::doMediaPrev(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPrev();
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

void Phone::onMediaConnect(MediaSource* src){
	mediaCurrent = src;
	Events::post(Facility::Phone, Event { .action = Event::MediaConnected });
}

void Phone::onMediaDisconnect(MediaSource* src){
	if(mediaCurrent != src) return;
	Events::post(Facility::Phone, Event { .action = Event::MediaDisconnected });
	mediaCurrent = nullptr;
}

void Phone::onMediaInfo(const Media& media){
	currentMedia.reset();

	currentMedia = media;
	Events::post(Facility::Phone, Event { .action = Event::MediaInfo });
}

void Phone::onMediaState(MediaState state){
	Events::post(Facility::Phone, Event { .action = Event::MediaState, .data = { .mediaState = getMediaState() } });
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
	if(current != &android) return;
	android.findPhoneStart();
}

void Phone::findPhoneStop(){
	if(current != &android) return;
	android.findPhoneStop();
}
