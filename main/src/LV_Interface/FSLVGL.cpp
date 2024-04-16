#include "FSLVGL.h"
#include <cstdio>
#include <dirent.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "Filepaths.hpp"

const char* TAG = "FSLVGL";
std::unordered_set<FSLVGL::FileResource, std::hash<RamFile*>> FSLVGL::cache;

static constexpr size_t CacheReserveSize = 85;

FSLVGL::FSLVGL(char letter){
	cache.reserve(CacheReserveSize + 16);

	esp_vfs_spiffs_conf_t conf = {
			.base_path = "/spiffs",
			.partition_label = "storage",
			.max_files = 8,
			.format_if_mount_failed = false
	};

	auto ret = esp_vfs_spiffs_register(&conf);
	if(ret != ESP_OK){
		if(ret == ESP_FAIL){
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}else if(ret == ESP_ERR_NOT_FOUND){
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}else{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}

		return;
	}


	lv_fs_drv_init(&drv);                     /*Basic initialization*/

	drv.letter = letter;                         /*An uppercase letter to identify the drive */
	drv.ready_cb = ready_cb;               /*Callback to tell if the drive is ready to use */
	drv.open_cb = open_cb;                 /*Callback to open a file */
	drv.close_cb = close_cb;               /*Callback to close a file */
	drv.read_cb = read_cb;                 /*Callback to read a file */
	drv.write_cb = write_cb;               /*Callback to write a file */
	drv.seek_cb = seek_cb;                 /*Callback to seek in a file (Move cursor) */
	drv.tell_cb = tell_cb;                 /*Callback to tell the cursor position  */

	drv.dir_open_cb = dir_open_cb;         /*Callback to open directory to read its content */
	drv.dir_read_cb = dir_read_cb;         /*Callback to read a directory's content */
	drv.dir_close_cb = dir_close_cb;       /*Callback to close a directory */

	drv.user_data = this;             /*Any custom data if required*/

	lv_fs_drv_register(&drv);                 /*Finally register the drive*/
}

FSLVGL::~FSLVGL(){
	esp_vfs_spiffs_unregister("storage");
}

auto FSLVGL::findCache(const std::string& lvPath){
	std::string path("/spiffs");
	path.append(lvPath);

	return std::find_if(cache.begin(), cache.end(), [&path](auto fileResource){
		return fileResource.ramFile->path() == path;
	});
}

auto FSLVGL::findCache(void* ptr){
	return std::find_if(cache.begin(), cache.end(), [ptr](auto fileResource){
		return fileResource.ramFile == ptr;
	});
}

void FSLVGL::addToCache(const char* path, bool use32bAligned){
	if(path == nullptr || std::string(path).empty()){
		return;
	}

	std::string p;
	if(strchr(path, DriveSeparator) != nullptr){
		p = std::string(path + 2);
	}else{
		p = path;
	}

	auto found = findCache(p);
	if(found != cache.end()){
		found->deleteFlag = false;
		return;
	}

	std::string spath("/spiffs");
	spath.append(p);

	auto ram = new RamFile(spath.c_str(), use32bAligned);
	if(ram->size() == 0){
		delete ram;
		return;
	}

	FileResource r = { ram, false };
	cache.insert(r);
}

void FSLVGL::removeFromCache(const char* path){
	std::string p;
	if(strchr(path, DriveSeparator) != nullptr){
		p = std::string(path + 2);
	}else{
		p = path;
	}

	auto it = findCache(p);
	if(it == cache.end()) return;

	it->deleteFlag = true;
}

void FSLVGL::loadCache(Theme theme){
	const char* Cached[] = {
			THEMED_FILE(LockScreen, Background, theme),
			THEMED_FILE(LockScreen, Phone, theme),
			THEMED_FILE(LockScreen, BatteryLevel, theme),
			THEMED_FILE(LockScreen, Clock, theme),
			THEMED_FILE(LockScreen, Battery1, theme),
			THEMED_FILE(LockScreen, Battery2, theme),
			THEMED_FILE(LockScreen, Battery3, theme),
			THEMED_FILE(LockScreen, Battery4, theme),
			THEMED_FILE(LockScreen, Charging, theme),

			THEMED_FILE(Clock, Num0, theme),
			THEMED_FILE(Clock, Num1, theme),
			THEMED_FILE(Clock, Num2, theme),
			THEMED_FILE(Clock, Num3, theme),
			THEMED_FILE(Clock, Num4, theme),
			THEMED_FILE(Clock, Num5, theme),
			THEMED_FILE(Clock, Num6, theme),
			THEMED_FILE(Clock, Num7, theme),
			THEMED_FILE(Clock, Num8, theme),
			THEMED_FILE(Clock, Num9, theme),
			THEMED_FILE(Clock, Colon, theme),
			THEMED_FILE(Clock, Space, theme),

			THEMED_FILE(Icons, AppInst, theme),
			THEMED_FILE(Icons, AppMess, theme),
			THEMED_FILE(Icons, AppSms, theme),
			THEMED_FILE(Icons, AppSnap, theme),
			THEMED_FILE(Icons, AppTiktok, theme),
			THEMED_FILE(Icons, AppWapp, theme),
			THEMED_FILE(Icons, Back, theme),
			THEMED_FILE(Icons, BackSel, theme),
			THEMED_FILE(Icons, CallIn, theme),
			THEMED_FILE(Icons, CallMiss, theme),
			THEMED_FILE(Icons, CallOut, theme),
			THEMED_FILE(Icons, CatEmail, theme),
			THEMED_FILE(Icons, CatEntert, theme),
			THEMED_FILE(Icons, CatFin, theme),
			THEMED_FILE(Icons, CatHealth, theme),
			THEMED_FILE(Icons, CatLoc, theme),
			THEMED_FILE(Icons, CatNews, theme),
			THEMED_FILE(Icons, CatOther, theme),
			THEMED_FILE(Icons, CatSched, theme),
			THEMED_FILE(Icons, CatSoc, theme),
			THEMED_FILE(Icons, LockOpen, theme),
			THEMED_FILE(Icons, LockClosed, theme),
			THEMED_FILE(Icons, Trash, theme),
			THEMED_FILE(Icons, TrashSel, theme),

			THEMED_FILE(SmallIcons, AppInst, theme),
			THEMED_FILE(SmallIcons, AppMess, theme),
			THEMED_FILE(SmallIcons, AppSms, theme),
			THEMED_FILE(SmallIcons, AppSnap, theme),
			THEMED_FILE(SmallIcons, AppTiktok, theme),
			THEMED_FILE(SmallIcons, AppWapp, theme),
			THEMED_FILE(SmallIcons, Back, theme),
			THEMED_FILE(SmallIcons, BackSel, theme),
			THEMED_FILE(SmallIcons, CallIn, theme),
			THEMED_FILE(SmallIcons, CallMiss, theme),
			THEMED_FILE(SmallIcons, CallOut, theme),
			THEMED_FILE(SmallIcons, CatEmail, theme),
			THEMED_FILE(SmallIcons, CatEntert, theme),
			THEMED_FILE(SmallIcons, CatFin, theme),
			THEMED_FILE(SmallIcons, CatHealth, theme),
			THEMED_FILE(SmallIcons, CatLoc, theme),
			THEMED_FILE(SmallIcons, CatNews, theme),
			THEMED_FILE(SmallIcons, CatOther, theme),
			THEMED_FILE(SmallIcons, CatSched, theme),
			THEMED_FILE(SmallIcons, CatSoc, theme),
			THEMED_FILE(SmallIcons, LockOpen, theme),
			THEMED_FILE(SmallIcons, LockClosed, theme),
			THEMED_FILE(SmallIcons, Trash, theme),
			THEMED_FILE(SmallIcons, TrashSel, theme),

			THEMED_FILE(Menu, BatteryEmpty, theme),
			THEMED_FILE(Menu, BatteryFull, theme),
			THEMED_FILE(Menu, BatteryLow, theme),
			THEMED_FILE(Menu, BatteryMid, theme),
			THEMED_FILE(Menu, BigLowBattery, theme),
			THEMED_FILE(Menu, Phone, theme),
			THEMED_FILE(Menu, PhoneDisconnected, theme),
			THEMED_FILE(Menu, Background, theme),
			THEMED_FILE(Menu, SettingsBackground, theme),
			THEMED_FILE(Menu, Connection, theme),
			THEMED_FILE(Menu, Find, theme),
			THEMED_FILE(Menu, Level, theme),
			THEMED_FILE(Menu, Settings, theme),
			THEMED_FILE(Menu, Theremin, theme),
			THEMED_FILE(Menu, Lunar, theme)
	};

	for(const auto& p: Cached){
		addToCache(p);
	}
}

void FSLVGL::unloadCache(){
	for(auto it = cache.begin(); it != cache.end();){
		it->deleteFlag = true;

		if(it->openCount == 0){
			delete it->ramFile;
			it = cache.erase(it);
		}else{
			++it;
		}
	}
}

bool FSLVGL::ready_cb(struct _lv_fs_drv_t* drv){
	return true;
}

void* FSLVGL::open_cb(struct _lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode){
	auto cached = findCache(path);
	if(cached != cache.end()){
		cached->openCount++;
		(*cached).ramFile->seek(0);
		return (*cached).ramFile;
	}

	const char* fsMode;

	if(mode == LV_FS_MODE_WR){
		fsMode = "w";
	}else{
		fsMode = "r";
	}

	auto fslvgl = (FSLVGL*) drv->user_data;
	std::string p = fslvgl->Root + std::string(path);

	return (void*) fopen(p.c_str(), fsMode);
}

lv_fs_res_t FSLVGL::close_cb(struct _lv_fs_drv_t* drv, void* file_p){
	auto it = findCache(file_p);
	if(it != cache.end()){
		if(it->openCount != 0){
			it->openCount--;
		}

		if(it->deleteFlag){
			delete it->ramFile;
			cache.erase(it);
		}
		return 0;
	}

	fclose((FILE*) file_p);
	return 0;
}

lv_fs_res_t FSLVGL::read_cb(struct _lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br){
	auto cached = findCache(file_p);
	if(cached != cache.end()){
		*br = (*cached).ramFile->read(buf, btr);
		return 0;
	}

	if(ferror((FILE*) file_p)) return LV_FS_RES_NOT_EX;
	*br = fread(buf, 1, btr, (FILE*) file_p);
	return 0;
}

lv_fs_res_t FSLVGL::write_cb(struct _lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw){
	auto cached = findCache(file_p);
	if(cached != cache.end()){
		*bw = 0;
		return 0;
	}

	if(ferror((FILE*) file_p)) return LV_FS_RES_NOT_EX;

	*bw = fwrite((uint8_t*) buf, 1, btw, (FILE*) file_p);
	return 0;
}

lv_fs_res_t FSLVGL::seek_cb(struct _lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence){
	auto cached = findCache(file_p);
	if(cached != cache.end()){
		static const std::unordered_map<lv_fs_whence_t, int> SeekMap = {
				{ LV_FS_SEEK_SET, SEEK_SET },
				{ LV_FS_SEEK_CUR, SEEK_CUR },
				{ LV_FS_SEEK_END, SEEK_END },
		};
		(*cached).ramFile->seek(pos, SeekMap.at(whence));
		return 0;
	}

	if(ferror((FILE*) file_p)){
		return LV_FS_RES_NOT_EX;
	}

	int mode;
	switch(whence){
		case LV_FS_SEEK_SET:
			mode = SEEK_SET;
			break;
		case LV_FS_SEEK_CUR:
			mode = SEEK_CUR;
			break;
		case LV_FS_SEEK_END:
			mode = SEEK_END;
			break;
		default:
			mode = SEEK_SET;
	}
	if(fseek((FILE*) file_p, pos, mode) != 0){
		return LV_FS_RES_INV_PARAM;
	}
	return 0;

}

lv_fs_res_t FSLVGL::tell_cb(struct _lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p){
	auto cached = findCache(file_p);
	if(cached != cache.end()){
		*pos_p = (*cached).ramFile->pos();
		return 0;
	}

	if(ferror((FILE*) file_p)) return LV_FS_RES_NOT_EX;
	auto val = ftell((FILE*) file_p);
	if(val == -1) return LV_FS_RES_UNKNOWN;
	*pos_p = val;
	return 0;
}

void* FSLVGL::dir_open_cb(struct _lv_fs_drv_t* drv, const char* path){
	auto fslvgl = (FSLVGL*) drv->user_data;
	std::string p = fslvgl->Root + std::string(path);
	DIR* dir = opendir(p.c_str());
	return dir;

}

lv_fs_res_t FSLVGL::dir_read_cb(struct _lv_fs_drv_t* drv, void* rddir_p, char* fn){
	auto entry = readdir((DIR*) rddir_p);
	if(!entry) return LV_FS_RES_NOT_EX;
	strncpy(fn, entry->d_name, 256);
	return 0;
}

lv_fs_res_t FSLVGL::dir_close_cb(struct _lv_fs_drv_t* drv, void* rddir_p){
	closedir((DIR*) rddir_p);
	return 0;
}
