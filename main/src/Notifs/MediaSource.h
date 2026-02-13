#ifndef CLOCKSTAR_FIRMWARE_MEDIASOURCE_H
#define CLOCKSTAR_FIRMWARE_MEDIASOURCE_H

#include "Media.h"
#include <functional>

class MediaSource {
public:

	using ConnectCB = std::function<void()>;
	using DisconnectCB = std::function<void()>;

	using MediaUpdateCB = std::function<void(Media media)>;

	void setOnConnect(ConnectCB onConnect);
	void setOnDisconnect(DisconnectCB onDisconnect);

	void setOnMediaUpdate(MediaUpdateCB onMediaUpdate);

	// Actions sent to the app
	virtual void mediaPlay() = 0;
	virtual void mediaPause() = 0;
	virtual void mediaNext() = 0;
	virtual void mediaPrev() = 0;

protected:

	void mediaUpdate(const Media& media);

private:

	ConnectCB onConnect;
	DisconnectCB onDisconnect;

	MediaUpdateCB onMediaUpdate;

};

#endif //CLOCKSTAR_FIRMWARE_MEDIASOURCE_H