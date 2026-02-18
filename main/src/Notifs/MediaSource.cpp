#include "MediaSource.h"

void MediaSource::connect() {
	if(onConnect) onConnect();
}

void MediaSource::disconnect() {
	if(onDisconnect) onDisconnect();
}

void MediaSource::mediaInfo(const Media& media) {
	if(onMediaInfo) onMediaInfo(media);
}

void MediaSource::mediaState(MediaState state) {
	if(onMediaState) onMediaState(state);
}

void MediaSource::setOnConnect(MediaSource::ConnectCB onConnect) {
	MediaSource::onConnect = std::move(onConnect);
}

void MediaSource::setOnDisconnect(MediaSource::DisconnectCB onDisconnect) {
	MediaSource::onDisconnect = std::move(onDisconnect);
}

void MediaSource::setOnMediaInfo(MediaSource::MediaInfoCB onMediaInfo) {
	MediaSource::onMediaInfo = std::move(onMediaInfo);
}

void MediaSource::setOnMediaState(MediaSource::MediaStateCB onMediaState) {
	MediaSource::onMediaState = std::move(onMediaState);
}