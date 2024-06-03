#include "RamFile.h"
#include <string.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

static const char* TAG = "RamFile";

RamFile::RamFile(const char* path, bool use32bAligned) : filePath(path){
	auto file = fopen(path, "rb");
	if(file == nullptr){
		ESP_LOGE(TAG, "Couldn't open file: %s", path);
		return;
	}

	fseek(file, 0L, SEEK_END);
	fileSize = ftell(file);
	rewind(file);

	if(fileSize == 0){
		ESP_LOGE(TAG, "File is empty: %s", path);
		fclose(file);
		return;
	}
	size_t allocSize = fileSize;
	if(use32bAligned){
		auto rest = fileSize % 4;
		if(rest != 0){
			allocSize += (4 - rest);
		}
	}

	data = (uint8_t*) heap_caps_malloc(allocSize, MALLOC_CAP_SPIRAM | (use32bAligned ? MALLOC_CAP_32BIT : MALLOC_CAP_8BIT));
	if(data == nullptr){
		fileSize = 0;
		ESP_LOGE(TAG, "Couldn't allocate memory for %s. Need %zu B, largest block: %zu B", path, allocSize,
				 heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | (use32bAligned ? MALLOC_CAP_32BIT : MALLOC_CAP_8BIT)));
		fclose(file);
		return;
	}

	fread(data, 1, fileSize, file);
	fclose(file);
}

RamFile::~RamFile(){
	free(data);
}

size_t IRAM_ATTR RamFile::read(void* dest, size_t len){
	if(cursor >= fileSize) return 0;
	len = std::min(len, fileSize - cursor);
	if(len <= 0) return 0;

	memcpy(dest, data + cursor, len);
	cursor += len;

	return len;
}

void RamFile::seek(size_t pos, int whence){
	if(whence == SEEK_SET){
		cursor = pos;
	}else if(whence == SEEK_CUR){
		cursor += pos;
	}else if(whence == SEEK_END){
		cursor = fileSize - pos;
	}
}

size_t RamFile::pos(){
	return cursor;
}

size_t RamFile::size(){
	return fileSize;
}

std::string RamFile::path(){
	return filePath;
}
