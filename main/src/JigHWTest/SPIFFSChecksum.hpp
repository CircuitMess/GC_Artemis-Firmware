#ifndef ARTEMIS_SPIFFS_CHECKSUM_HPP
#define ARTEMIS_SPIFFS_CHECKSUM_HPP

struct {
	const char* name;
	uint32_t sum;
} static const SPIFFSChecksums[] PROGMEM = {
	{ "/spiffs/bg.bin", 1218486},
	{ "/spiffs/bg_bot.bin", 1519635},
	{ "/spiffs/clockIcons/0.bin", 10867},
	{ "/spiffs/clockIcons/1.bin", 7052},
	{ "/spiffs/clockIcons/2.bin", 11855},
	{ "/spiffs/clockIcons/3.bin", 10293},
	{ "/spiffs/clockIcons/4.bin", 8545},
	{ "/spiffs/clockIcons/5.bin", 11855},
	{ "/spiffs/clockIcons/6.bin", 12407},
	{ "/spiffs/clockIcons/7.bin", 5473},
	{ "/spiffs/clockIcons/8.bin", 11261},
	{ "/spiffs/clockIcons/9.bin", 11477},
	{ "/spiffs/clockIcons/colon.bin", 3152},
	{ "/spiffs/clockIcons/space.bin", 254},
	{ "/spiffs/icon/app_inst.bin", 2055},
	{ "/spiffs/icon/app_mess.bin", 2091},
	{ "/spiffs/icon/app_sms.bin", 2647},
	{ "/spiffs/icon/app_snap.bin", 2824},
	{ "/spiffs/icon/app_tiktok.bin", 1342},
	{ "/spiffs/icon/app_wapp.bin", 2073},
	{ "/spiffs/icon/back.bin", 2117},
	{ "/spiffs/icon/back_sel.bin", 1749},
	{ "/spiffs/icon/call_in.bin", 1807},
	{ "/spiffs/icon/call_miss.bin", 2005},
	{ "/spiffs/icon/call_out.bin", 1807},
	{ "/spiffs/icon/cat_email.bin", 2594},
	{ "/spiffs/icon/cat_entert.bin", 1543},
	{ "/spiffs/icon/cat_fin.bin", 1676},
	{ "/spiffs/icon/cat_health.bin", 1677},
	{ "/spiffs/icon/cat_loc.bin", 1373},
	{ "/spiffs/icon/cat_news.bin", 3446},
	{ "/spiffs/icon/cat_other.bin", 1323},
	{ "/spiffs/icon/cat_sched.bin", 2853},
	{ "/spiffs/icon/cat_soc.bin", 1514},
	{ "/spiffs/icon/etc.bin", 1855},
	{ "/spiffs/icon/lock_closed.bin", 1865},
	{ "/spiffs/icon/lock_open.bin", 1869},
	{ "/spiffs/icon/trash.bin", 2454},
	{ "/spiffs/icon/trash_sel.bin", 2086},
	{ "/spiffs/icons/batteryEmpty.bin", 3139},
	{ "/spiffs/icons/batteryFull.bin", 3706},
	{ "/spiffs/icons/batteryLow.bin", 3283},
	{ "/spiffs/icons/batteryMid.bin", 3670},
	{ "/spiffs/icons/bigLowBattery.bin", 20505},
	{ "/spiffs/icons/phone.bin", 2045},
	{ "/spiffs/icons/phoneDisconnected.bin", 1933},
	{ "/spiffs/intro/artemis.bin", 10659},
	{ "/spiffs/intro/blackBg.bin", 1932},
	{ "/spiffs/intro/cm.bin", 126043},
	{ "/spiffs/intro/geek.bin", 68281},
	{ "/spiffs/intro/orangeBg.bin", 348012},
	{ "/spiffs/intro/space.bin", 24013},
	{ "/spiffs/level/bg.bin", 212282},
	{ "/spiffs/level/bubble.bin", 8027},
	{ "/spiffs/level/markingsCenter.bin", 5087},
	{ "/spiffs/level/markingsHorizontal.bin", 4917},
	{ "/spiffs/level/markingsVertical.bin", 4161},
	{ "/spiffs/lockbg.bin", 27315},
	{ "/spiffs/menu/bg.bin", 14499},
	{ "/spiffs/menu/connection.bin", 14371},
	{ "/spiffs/menu/find.bin", 15910},
	{ "/spiffs/menu/level.bin", 17134},
	{ "/spiffs/menu/settings.bin", 27264},
	{ "/spiffs/menu/theremin.bin", 12632},
	{ "/spiffs/ModalBg.bin", 240652},
	{ "/spiffs/theremin/bg.bin", 362560},
	{ "/spiffs/theremin/dotHorizontal.bin", 1854},
	{ "/spiffs/theremin/dotVertical.bin", 2952},
};

#endif //ARTEMIS_SPIFFS_CHECKSUM_HPP