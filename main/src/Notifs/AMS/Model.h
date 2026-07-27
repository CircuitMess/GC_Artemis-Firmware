#ifndef CLOCKSTAR_FIRMWARE_AMS_MODEL_H
#define CLOCKSTAR_FIRMWARE_AMS_MODEL_H

#include <cstdint>

namespace AMS {

	enum EntityID : uint8_t {
		Player = 0,
		Queue = 1,
		Track = 2,
	};

	enum PlayerAttributeID : uint8_t {
		PlayerName = 0,
		PlayerPlaybackInfo = 1,
		PlayerVolume = 2,
	};

	enum QueueAttributeID : uint8_t {
		QueueIndex = 0,
		QueueCount = 1,
		QueueShuffleMode = 2,
		QueueRepeatMode = 3,
	};

	enum TrackAttributeID : uint8_t {
		TrackArtist = 0,
		TrackAlbum = 1,
		TrackTitle = 2,
		TrackDuration = 3,
	};

	enum RemoteCommandID : uint8_t {
		Play = 0,
		Pause = 1,
		TogglePlayPause = 2,
		NextTrack = 3,
		PreviousTrack = 4,
		VolumeUp = 5,
		VolumeDown = 6,
		AdvanceRepeatMode = 7,
		AdvanceShuffleMode = 8,
		SkipForward = 9,
		SkipBackward = 10,
		LikeTrack = 11,
		DislikeTrack = 12,
		BookmarkTrack = 13,
	};

	enum EntityUpdateFlags : uint8_t {
		Truncated = 1 << 0,
	};

	enum PlaybackState : uint8_t {
		StatePaused = 0,
		StatePlaying = 1,
		StateRewinding = 2,
		StateFastForwarding = 3,
	};

}

#endif //CLOCKSTAR_FIRMWARE_AMS_MODEL_H
