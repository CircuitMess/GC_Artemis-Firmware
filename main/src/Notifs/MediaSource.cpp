#include "MediaSource.h"

void MediaSource::mediaUpdate(Media media) {
	if(onMediaUpdate) onMediaUpdate(media);
}

void MediaSource::setOnConnect(MediaSource::ConnectCB onConnect) {
	MediaSource::onConnect = std::move(onConnect);
}

void MediaSource::setOnDisconnect(MediaSource::DisconnectCB onDisconnect) {
	MediaSource::onDisconnect = std::move(onDisconnect);
}

void MediaSource::setOnMediaUpdate(MediaSource::MediaUpdateCB onMediaUpdate) {
	MediaSource::onMediaUpdate = std::move(onMediaUpdate);
}
