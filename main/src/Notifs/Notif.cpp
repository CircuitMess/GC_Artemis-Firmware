#include "Notif.h"
#include <unordered_map>
#include "Filepaths.hpp"
#include "Settings/Settings.h"
#include "Util/Services.h"

NotifIcon notifIcon(const Notif& notif){
	const std::unordered_map<std::string, NotifIcon> appMap = {
			{ "Messenger", NotifIcon::Messenger },
			{ "WhatsApp",  NotifIcon::WhatsApp },
			{ "Messages",  NotifIcon::Messages },
			{ "Instagram", NotifIcon::Instagram },
			{ "Snapchat",  NotifIcon::Snapchat },
			{ "TikTok",    NotifIcon::TikTok }
	};

	const std::unordered_map<Notif::Category, NotifIcon> catMap = {
			{ Notif::Category::Other,              NotifIcon::Other },
			{ Notif::Category::IncomingCall,       NotifIcon::IncomingCall },
			{ Notif::Category::MissedCall,         NotifIcon::MissedCall },
			{ Notif::Category::Voicemail,          NotifIcon::Voicemail },
			{ Notif::Category::Social,             NotifIcon::Social },
			{ Notif::Category::Schedule,           NotifIcon::Schedule },
			{ Notif::Category::Email,              NotifIcon::Email },
			{ Notif::Category::News,               NotifIcon::News },
			{ Notif::Category::HealthAndFitness,   NotifIcon::HealthAndFitness },
			{ Notif::Category::BusinessAndFinance, NotifIcon::BusinessAndFinance },
			{ Notif::Category::Location,           NotifIcon::Location },
			{ Notif::Category::Entertainment,      NotifIcon::Entertainment },
			{ Notif::Category::OutgoingCall,       NotifIcon::OutgoingCall }
	};

	auto itApp = appMap.find(notif.appID);
	if(itApp != appMap.end()){
		return itApp->second;
	}

	auto itCat = catMap.find(notif.category);
	if(itCat != catMap.end()){
		return itCat->second;
	}

	return NotifIcon::Other;
}

const char* iconPath(NotifIcon icon, bool smallIcon){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return "";
	}

	const Theme theme = settings->get().themeData.theme;

	const std::unordered_map<NotifIcon, const char*> pathMap = {
			{ NotifIcon::Messenger,          smallIcon ? THEMED_FILE(SmallIcons, AppMess, theme) : THEMED_FILE(Icons, AppMess, theme) },
			{ NotifIcon::WhatsApp,           smallIcon ? THEMED_FILE(SmallIcons, AppWapp, theme) : THEMED_FILE(Icons, AppWapp, theme) },
			{ NotifIcon::Messages,           smallIcon ? THEMED_FILE(SmallIcons, AppSms, theme) : THEMED_FILE(Icons, AppSms, theme) },
			{ NotifIcon::Instagram,          smallIcon ? THEMED_FILE(SmallIcons, AppInst, theme) : THEMED_FILE(Icons, AppInst, theme) },
			{ NotifIcon::Snapchat,           smallIcon ? THEMED_FILE(SmallIcons, AppSnap, theme) : THEMED_FILE(Icons, AppSnap, theme) },
			{ NotifIcon::TikTok,             smallIcon ? THEMED_FILE(SmallIcons, AppTiktok, theme) : THEMED_FILE(Icons, AppTiktok, theme) },
			{ NotifIcon::Other,              smallIcon ? THEMED_FILE(SmallIcons, CatOther, theme) : THEMED_FILE(Icons, CatOther, theme) },
			{ NotifIcon::IncomingCall,       smallIcon ? THEMED_FILE(SmallIcons, CallIn, theme) : THEMED_FILE(Icons, CallIn, theme) },
			{ NotifIcon::MissedCall,         smallIcon ? THEMED_FILE(SmallIcons, CallMiss, theme) : THEMED_FILE(Icons, CallMiss, theme) },
			{ NotifIcon::Voicemail,          smallIcon ? THEMED_FILE(SmallIcons, CatOther, theme) : THEMED_FILE(Icons, CatOther, theme) },
			{ NotifIcon::Social,             smallIcon ? THEMED_FILE(SmallIcons, CatSoc, theme) : THEMED_FILE(Icons, CatSoc, theme) },
			{ NotifIcon::Schedule,           smallIcon ? THEMED_FILE(SmallIcons, CatSched, theme) : THEMED_FILE(Icons, CatSched, theme) },
			{ NotifIcon::Email,              smallIcon ? THEMED_FILE(SmallIcons, CatEmail, theme) : THEMED_FILE(Icons, CatEmail, theme) },
			{ NotifIcon::News,               smallIcon ? THEMED_FILE(SmallIcons, CatNews, theme) : THEMED_FILE(Icons, CatNews, theme) },
			{ NotifIcon::HealthAndFitness,   smallIcon ? THEMED_FILE(SmallIcons, CatHealth, theme) : THEMED_FILE(Icons, CatHealth, theme) },
			{ NotifIcon::BusinessAndFinance, smallIcon ? THEMED_FILE(SmallIcons, CatFin, theme) : THEMED_FILE(Icons, CatFin, theme) },
			{ NotifIcon::Location,           smallIcon ? THEMED_FILE(SmallIcons, CatLoc, theme) : THEMED_FILE(Icons, CatLoc, theme) },
			{ NotifIcon::Entertainment,      smallIcon ? THEMED_FILE(SmallIcons, CatEntert, theme) : THEMED_FILE(Icons, CatEntert, theme) },
			{ NotifIcon::OutgoingCall,       smallIcon ? THEMED_FILE(SmallIcons, CallOut, theme) : THEMED_FILE(Icons, CallOut, theme) }
	};

	auto it = pathMap.find(icon);
	if(it != pathMap.end()){
		return it->second;
	}

	return pathMap.at(NotifIcon::Other);
}

