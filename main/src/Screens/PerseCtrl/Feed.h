#ifndef PERSE_MISSIONCTRL_FEED_H
#define PERSE_MISSIONCTRL_FEED_H

#include "UDPListener.h"
#include "DriveInfo.h"
#include "Util/Threaded.h"
#include <semaphore>
#include <atomic>

typedef uint16_t Color;

class Feed {
public:
	Feed();
	virtual ~Feed();

	bool nextFrame(std::function<void(const DriveInfo& info, const Color* img)>);

	void setPostProcCallback(std::function<void(const DriveInfo&, Color*)> callback);

	void clearFrame();

private:
	std::unique_ptr<class JPEGDEC> jpeg;
	UDPListener udp;

	RingBuffer rxBuf;
	std::mutex rxMut;

	std::binary_semaphore dataAvailable;

	ThreadedClosure readTask;
	uint8_t* readBuf;
	static constexpr size_t ReadBufSize = 4096;
	void readLoop();

	ThreadedClosure decodeTask;
	void decodeLoop();
	bool findFrame();

	bool freeImgs[3] = { true, true, true };
	Color* frameImgs[3];

	std::mutex readyFrameMut;
	struct {
		DriveInfo info;
		int imgIndex = -1;
	} readyFrame;

	constexpr static size_t JpgMaxSize = 8000; //upper size limit for JPG quality 30 on Batmobile camera, approximately
	constexpr static size_t RxBufSize = 3 * (sizeof(DriveInfo) + JpgMaxSize);

	std::function<void(const DriveInfo&, Color* frame)> postProcCallback;
};


#endif //PERSE_MISSIONCTRL_FEED_H
