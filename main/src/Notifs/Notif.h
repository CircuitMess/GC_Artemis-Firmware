#ifndef CLOCKSTAR_FIRMWARE_NOTIF_H
#define CLOCKSTAR_FIRMWARE_NOTIF_H

#include <string>
#include <ctime>

struct Notif {
	uint32_t uid;

	std::string title;
	//std::string subtitle;
	std::string message;

	std::string appID;
	//std::tm time;

	/*struct {
		std::string pos;
		std::string neg;
	} label;*/

	enum class Category {
		Other,
		IncomingCall,
		MissedCall,
		Voicemail,
		Social,
		Schedule,
		Email,
		News,
		HealthAndFitness,
		BusinessAndFinance,
		Location,
		Entertainment,
		OutgoingCall
	} category;
};

enum class NotifIcon : uint8_t {
	//Apps
	Messenger,
	WhatsApp,
	Messages,
	Instagram,
	Snapchat,
	TikTok,

	//Categories
	Other,
	IncomingCall,
	MissedCall,
	Voicemail,
	Social,
	Schedule,
	Email,
	News,
	HealthAndFitness,
	BusinessAndFinance,
	Location,
	Entertainment,
	OutgoingCall
};

NotifIcon notifIcon(const Notif& notif);
const char* iconPath(NotifIcon icon, bool smallIcon = false);

#endif //CLOCKSTAR_FIRMWARE_NOTIF_H
