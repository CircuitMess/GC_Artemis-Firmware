#ifndef CLOCKSTAR_FIRMWARE_MEDIA_H
#define CLOCKSTAR_FIRMWARE_MEDIA_H

#include <string>
#include <cstdint>

enum class MediaState : uint8_t {
	Stopped,
	Playing,
	Paused
};

struct Media {
	uint32_t uid = 0;
	MediaState state = MediaState::Stopped;

	std::string title;
	std::string artist;
	std::string album;
	std::string appID;
};

#endif //CLOCKSTAR_FIRMWARE_MEDIA_H