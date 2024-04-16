#include "Notif.h"
#include <unordered_map>
#include "Filepaths.hpp"
#include "Settings/Settings.h"
#include "Util/Services.h"

const char* iconPath(const Notif& notif, bool smallIcon){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return "";
	}

	const Theme theme = settings->get().themeData.theme;

	const std::unordered_map<std::string, const char*> appMap = {
			{ "Messenger", smallIcon ? THEMED_FILE(SmallIcons, AppMess, theme) : THEMED_FILE(Icons, AppMess, theme) },
			{ "WhatsApp",  smallIcon ? THEMED_FILE(SmallIcons, AppWapp, theme) : THEMED_FILE(Icons, AppWapp, theme) },
			{ "Messages",  smallIcon ? THEMED_FILE(SmallIcons, AppSms, theme) : THEMED_FILE(Icons, AppSms, theme) },
			{ "Instagram", smallIcon ? THEMED_FILE(SmallIcons, AppInst, theme) : THEMED_FILE(Icons, AppInst, theme) },
			{ "Snapchat",  smallIcon ? THEMED_FILE(SmallIcons, AppSnap, theme) : THEMED_FILE(Icons, AppSnap, theme) },
			{ "TikTok",    smallIcon ? THEMED_FILE(SmallIcons, AppTiktok, theme) : THEMED_FILE(Icons, AppTiktok, theme) }
	};

	const std::unordered_map<Notif::Category, const char*> catMap = {
			{ Notif::Category::Other,              smallIcon ? THEMED_FILE(SmallIcons, CatOther, theme) : THEMED_FILE(Icons, CatOther, theme) },
			{ Notif::Category::IncomingCall,       smallIcon ? THEMED_FILE(SmallIcons, CallIn, theme) : THEMED_FILE(Icons, CallIn, theme) },
			{ Notif::Category::MissedCall,         smallIcon ? THEMED_FILE(SmallIcons, CallMiss, theme) : THEMED_FILE(Icons, CallMiss, theme) },
			{ Notif::Category::Voicemail,          smallIcon ? THEMED_FILE(SmallIcons, CatOther, theme) : THEMED_FILE(Icons, CatOther, theme) },
			{ Notif::Category::Social,             smallIcon ? THEMED_FILE(SmallIcons, CatSoc, theme) : THEMED_FILE(Icons, CatSoc, theme) },
			{ Notif::Category::Schedule,           smallIcon ? THEMED_FILE(SmallIcons, CatSched, theme) : THEMED_FILE(Icons, CatSched, theme) },
			{ Notif::Category::Email,              smallIcon ? THEMED_FILE(SmallIcons, CatEmail, theme) : THEMED_FILE(Icons, CatEmail, theme) },
			{ Notif::Category::News,               smallIcon ? THEMED_FILE(SmallIcons, CatNews, theme) : THEMED_FILE(Icons, CatNews, theme) },
			{ Notif::Category::HealthAndFitness,   smallIcon ? THEMED_FILE(SmallIcons, CatHealth, theme) : THEMED_FILE(Icons, CatHealth, theme) },
			{ Notif::Category::BusinessAndFinance, smallIcon ? THEMED_FILE(SmallIcons, CatFin, theme) : THEMED_FILE(Icons, CatFin, theme) },
			{ Notif::Category::Location,           smallIcon ? THEMED_FILE(SmallIcons, CatLoc, theme) : THEMED_FILE(Icons, CatLoc, theme) },
			{ Notif::Category::Entertainment,      smallIcon ? THEMED_FILE(SmallIcons, CatEntert, theme) : THEMED_FILE(Icons, CatEntert, theme) },
			{ Notif::Category::OutgoingCall,       smallIcon ? THEMED_FILE(SmallIcons, CallOut, theme) : THEMED_FILE(Icons, CallOut, theme) }
	};

	auto itApp = appMap.find(notif.appID);
	if(itApp != appMap.end()){
		return itApp->second;
	}

	auto itCat = catMap.find(notif.category);
	if(itCat != catMap.end()){
		return itCat->second;
	}

	return THEMED_FILE(Icons, CatOther, theme);
}
