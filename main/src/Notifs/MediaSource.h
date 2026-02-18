#ifndef CLOCKSTAR_FIRMWARE_MEDIASOURCE_H
#define CLOCKSTAR_FIRMWARE_MEDIASOURCE_H

#include "Media.h"
#include <functional>

class MediaSource {
public:

	using ConnectCB = std::function<void()>;
	using DisconnectCB = std::function<void()>;

	using MediaInfoCB = std::function<void(Media media)>;
	using MediaStateCB = std::function<void(MediaState state)>;

	void setOnConnect(ConnectCB onConnect);
	void setOnDisconnect(DisconnectCB onDisconnect);

	void setOnMediaInfo(MediaInfoCB onMediaInfo);
	void setOnMediaState(MediaStateCB onMediaState);

	// Actions sent to the app
	virtual void mediaPlay() = 0;
	virtual void mediaPause() = 0;
	virtual void mediaNext() = 0;
	virtual void mediaPrev() = 0;

protected:

	void connect();
	void disconnect();

	void mediaInfo(const Media& media);
	void mediaState(MediaState state);

private:

	ConnectCB onConnect;
	DisconnectCB onDisconnect;

	MediaInfoCB onMediaInfo;
	MediaStateCB onMediaState;
};

#endif //CLOCKSTAR_FIRMWARE_MEDIASOURCE_H