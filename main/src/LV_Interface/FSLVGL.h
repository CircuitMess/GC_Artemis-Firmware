#ifndef CLOCKSTAR_FIRMWARE_FSLVGL_H
#define CLOCKSTAR_FIRMWARE_FSLVGL_H

#include <lvgl.h>
#include <string>
#include <unordered_set>
#include <memory>
#include "Util/RamFile.h"
#include "Settings/Settings.h"

class FSLVGL {
public:
	FSLVGL(char letter);
	virtual ~FSLVGL();

	/**
	 * Allocates memory and stores a file in memory.
	 * @param path Relative to /spiffs (e.g. /bg.bin)
	 * @param use32bAligned Choose to use ESP32's IRAM or not. Defaults to False.
	 * 						Useful if caching 4-byte palette sprites for example, but will throw LoadStoreError if data isn't accesed in 32-bit chunks.
	 */
	static void addToCache(const char* path, bool use32bAligned = false);
	static void removeFromCache(const char* path);

	static void loadCache(Theme theme);
	static void unloadCache();

private:
	lv_fs_drv_t drv;                   /*Needs to be static or global*/
	const std::string Root = "/spiffs";
	static constexpr const char DriveSeparator = ':';

	struct FileResource {
		RamFile* ramFile;
		mutable bool deleteFlag;
		mutable size_t openCount = 0;

		bool operator==(const FileResource& other) const{
			return other.ramFile == ramFile;
		}

		operator RamFile*() const{
			return ramFile;
		}
	};

	static std::unordered_set<FileResource, std::hash<RamFile*>> cache;

	static auto findCache(const std::string& path);
	static auto findCache(void* ptr);

	static bool ready_cb(struct _lv_fs_drv_t* drv);
	static void* open_cb(struct _lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode);
	static lv_fs_res_t close_cb(struct _lv_fs_drv_t* drv, void* file_p);
	static lv_fs_res_t read_cb(struct _lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br);
	static lv_fs_res_t write_cb(struct _lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw);
	static lv_fs_res_t seek_cb(struct _lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence);
	static lv_fs_res_t tell_cb(struct _lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p);
	static void* dir_open_cb(struct _lv_fs_drv_t* drv, const char* path);
	static lv_fs_res_t dir_read_cb(struct _lv_fs_drv_t* drv, void* rddir_p, char* fn);
	static lv_fs_res_t dir_close_cb(struct _lv_fs_drv_t* drv, void* rddir_p);
};


#endif //CLOCKSTAR_FIRMWARE_FSLVGL_H
