#include "Client.h"
#include "Util/stdafx.h"
#include "Util/TextSanitize.h"
#include <esp_log.h>

static const char* TAG = "AMS";

AMS::Client::Client(BLE::Client* client) : updateThread([this](){ loopUpdate(); }, "AMS::Update", 3 * 1024){
	service = client->addService(ServiceUUID);
	chr.remoteCommand = service->addChar(Char_RemoteCommand_UUID, ESP_GATT_CHAR_PROP_BIT_WRITE);
	chr.entityUpdate = service->addChar(Char_EntityUpdate_UUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_WRITE);

	service->setOnConnectCb([this](){ onConn(); });
	service->setOnDisconnectCb([this](){ onDiscon(); });

	chr.entityUpdate->setOnConnectedCb([this](){
		chr.entityUpdate->writeDescr(ESP_GATT_UUID_CHAR_CLIENT_CONFIG, { 0x01, 0x00 });
		registerEntities();
	});

	updateThread.start();
}

AMS::Client::~Client(){
	updateThread.stop();
}

void AMS::Client::onConn(){
	connected = true;
	connect();
}

void AMS::Client::onDiscon(){
	connected = false;

	{
		std::lock_guard lock(mut);
		info = MediaInfo{};
		hint.reset();
		state = MediaState::Stopped;
		dirty = false;
	}

	disconnect();
}

void AMS::Client::mediaPlay(){
	sendCommand(RemoteCommandID::Play);
}

void AMS::Client::mediaPause(){
	sendCommand(RemoteCommandID::Pause);
}

void AMS::Client::mediaNext(){
	sendCommand(RemoteCommandID::NextTrack);
}

void AMS::Client::mediaPrev(){
	sendCommand(RemoteCommandID::PreviousTrack);
}

void AMS::Client::sendCommand(RemoteCommandID cmd){
	if(!connected) return;
	ESP_LOGI(TAG, "Sending remote command %d", cmd);
	chr.remoteCommand->write({ static_cast<uint8_t>(cmd) });
}

void AMS::Client::registerEntities(){
	// Per AMS spec, registering with a write to Entity Update char tells the source
	// which attributes of which entity we want updates for. One write per entity.
	const std::vector<uint8_t> player = { EntityID::Player, PlayerAttributeID::PlayerName, PlayerAttributeID::PlayerPlaybackInfo };
	chr.entityUpdate->write(player);

	const std::vector<uint8_t> track = { EntityID::Track, TrackAttributeID::TrackTitle, TrackAttributeID::TrackArtist, TrackAttributeID::TrackAlbum };
	chr.entityUpdate->write(track);
}

void AMS::Client::loopUpdate(){
	if(chr.entityUpdate == nullptr || !connected){
		delayMillis(1000);
		return;
	}

	// When mediaInfo is pending, wake up after BatchTimeout to emit it as one batch.
	// MediaState emission is eager in processEntityUpdate and doesn't go through here.
	auto notif = chr.entityUpdate->getNextNotif(dirty ? BatchTimeout : portMAX_DELAY);
	if(!connected) return;

	if(notif == nullptr){
		if(dirty) flush();
		return;
	}

	processEntityUpdate(notif->data);
}

void AMS::Client::processEntityUpdate(const PSRAMByteBuffer& data){
	// Entity Update notification: [EntityID][AttributeID][Flags][value bytes...]
	// Only updates raw inputs (info, hint) and the dirty flag here; state is
	// derived and emitted in flush(). Track-empty observations reset `hint` so
	// a stale Paused/Playing from an idle iPhone period can't leak into the
	// next active session.
	if(data.size() < 3) return;

	const uint8_t entity = data[0];
	const uint8_t attr = data[1];
	// Truncation (data[2] & EntityUpdateFlags::Truncated) is ignored — Entity Attribute
	// char fetches the full value if needed; not implemented yet.

	const std::string_view value(reinterpret_cast<const char*>(data.data()) + 3, data.size() - 3);

	std::lock_guard lock(mut);

	if(entity == EntityID::Player){
		if(attr == PlayerAttributeID::PlayerName){
			sanitizeToAscii(value, info.appID);
			dirty = true;
		}else if(attr == PlayerAttributeID::PlayerPlaybackInfo){
			// CSV: <state>,<rate>,<elapsedTime> — only the first comma-separated token is needed.
			if(value.empty() || value[0] < '0' || value[0] > '9') return;
			const uint8_t sv = value[0] - '0';
			MediaState target;
			if(sv == PlaybackState::StatePaused)              target = MediaState::Paused;
			else if(sv == PlaybackState::StatePlaying)        target = MediaState::Playing;
			else if(sv == PlaybackState::StateRewinding)      target = MediaState::Playing;
			else if(sv == PlaybackState::StateFastForwarding) target = MediaState::Playing;
			else{
				ESP_LOGW(TAG, "Unknown playback state value: %d", sv);
				return;
			}

			// Only mark dirty when hint actually changes — sustained playback
			// re-sends PlayerPlaybackInfo every ~1s for elapsed-time ticks; we
			// don't want to schedule a flush (and re-emit mediaInfo) for those.
			if(!hint.has_value() || *hint != target){
				hint = target;
				dirty = true;
			}
		}
	}else if(entity == EntityID::Track){
		if(attr == TrackAttributeID::TrackTitle)       sanitizeToAscii(value, info.title);
		else if(attr == TrackAttributeID::TrackArtist) sanitizeToAscii(value, info.artist);
		else if(attr == TrackAttributeID::TrackAlbum)  sanitizeToAscii(value, info.album);
		else return;
		dirty = true;

		// Track-empty observation: invalidate hint so a stale Paused/Playing
		// from an idle iPhone period can't leak into the next active session.
		if(info.title.empty() && info.artist.empty() && info.album.empty()){
			hint.reset();
		}
	}
}

void AMS::Client::flush(){
	// Derive state from current inputs and emit mediaInfo before mediaState so
	// consumers see updated track metadata at or before the state transition.
	// State emission is gated on change; mediaInfo emission is unconditional
	// per flush (consumer-side coalescing is its concern).
	MediaInfo infoSnapshot;
	std::optional<MediaState> stateToEmit;
	{
		std::lock_guard lock(mut);
		dirty = false;
		infoSnapshot = info;

		const bool empty = info.title.empty() && info.artist.empty() && info.album.empty();
		std::optional<MediaState> newState;
		if(empty){
			newState = MediaState::Stopped;
		}else if(hint.has_value()){
			newState = *hint;
		}
		// else: track non-empty + no hint (PlayerPlaybackInfo hasn't arrived
		// yet for this session) → defer state emission. The next flush, once
		// PlayerPlaybackInfo lands, will emit the correct state.

		if(newState.has_value() && *newState != state){
			state = *newState;
			stateToEmit = state;
		}
	}

	if(connected) mediaInfo(infoSnapshot);
	if(stateToEmit.has_value() && connected) mediaState(*stateToEmit);
}
