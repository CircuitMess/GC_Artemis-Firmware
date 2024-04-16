#include "Feed.h"
#include "Util/stdafx.h"
#include <esp_log.h>
#include <mutex>
#include <JPEGDEC.h>

static const char* tag = "Feed";

Feed::Feed() : jpeg(std::make_unique<JPEGDEC>()), rxBuf(RxBufSize), dataAvailable(0), readTask([this](){ readLoop(); }, "FeedRead", 4096, 5, 0),
			   decodeTask([this](){ decodeLoop(); }, "FeedDecode", 4096, 5, 0){

	readBuf.resize(ReadBufSize);
	for(int i = 0; i < 3; i++){
		frameImgs[i].resize(160 * 120);
	}

	readTask.start();
	decodeTask.start();
}

Feed::~Feed(){
	readTask.stop();
	decodeTask.stop(0);
	dataAvailable.release();
	while(decodeTask.running()){
		delayMillis(1);
	}
}

void Feed::setPostProcCallback(std::function<void(const DriveInfo&, Color*)> callback){
	postProcCallback = callback;
}

bool Feed::nextFrame(std::function<void(const DriveInfo& info, const Color* img)> cb){
	std::unique_lock lock(readyFrameMut);
	if(readyFrame.imgIndex == -1) return false;

	volatile int frameIndex = readyFrame.imgIndex;
	if(frameIndex == -1) return false;

	volatile const DriveInfo info = readyFrame.info;
	volatile const Color* img = frameImgs[frameIndex].data();

	readyFrame.imgIndex = -1;
	lock.unlock();

	cb((const DriveInfo&) info, (const Color*) img);

	freeImgs[frameIndex] = true;

	return true;
}

void Feed::readLoop(){
	int bytes = udp.read(readBuf.data(), readBuf.size());
	if(bytes <= 0){
		delayMillis(10);
		return;
	}

	std::lock_guard lock(rxMut);
	rxBuf.write(readBuf.data(), bytes);
	dataAvailable.release();
}

void Feed::decodeLoop(){
	dataAvailable.acquire();

	std::unique_lock lock(rxMut);
	if(!findFrame()) return;

	rxBuf.skip(sizeof(FrameHeader));
	size_t size;
	rxBuf.read(reinterpret_cast<uint8_t*>(&size), sizeof(size_t));
	rxBuf.skip(sizeof(size_t));

	size_t available = rxBuf.readAvailable();
	auto frame = DriveInfo::deserialize(rxBuf, size);

	size_t readTotal = available - rxBuf.readAvailable();
	rxBuf.skip(size - readTotal); // skip frame if deserialize exited early

	rxBuf.skip(sizeof(FrameTrailer));

	lock.unlock();

    if(frame == nullptr || frame->frame.size == 0 || frame->frame.data == nullptr){
        return;
    }

	int freeImg = -1;
	for(int i = 0; i < 3; i++){
		if(freeImgs[i]){
			freeImg = i;
			break;
		}
	}
	if(freeImg == -1) return;

	freeImgs[freeImg] = false;
	auto imgBuf = frameImgs[freeImg].data();

	jpeg->openRAM((uint8_t*) (frame->frame.data), frame->frame.size, [](JPEGDRAW* data) -> int{
		for(int y = data->y, iy = 0; y < data->y + data->iHeight; y++, iy++){
			size_t offset = y * 160 + data->x;
			size_t ioffset = iy * data->iWidth;
			memcpy((uint8_t*) data->pUser + offset * 2, (uint8_t*) data->pPixels + ioffset * 2, data->iWidth * 2);
		}
		return 1;
	});

	jpeg->setUserPointer(imgBuf);
	jpeg->setPixelType(RGB565_BIG_ENDIAN);

	if(jpeg->decode(0, 0, 0) == 0){
		ESP_LOGE(tag, "decode error: %d", jpeg->getLastError());
		freeImgs[freeImg] = true;
		return;
	}

	free(frame->frame.data);
	frame->frame.data = nullptr;
	frame->frame.size = 0;

	if(postProcCallback){
		postProcCallback(*frame, imgBuf);
	}

	std::lock_guard frameLock(readyFrameMut);
	readyFrame.info = *frame;
	readyFrame.imgIndex = freeImg;
}

bool Feed::findFrame(){
	if(rxBuf.readAvailable() < sizeof(FrameHeader)) return false;

	// Search for frame header
	size_t bytesRead = 0;
	size_t bytesMatched = 0;
	for(; bytesRead < rxBuf.readAvailable(); bytesRead++){
		uint8_t byte = *rxBuf.peek<uint8_t>(bytesRead);

		if(byte == FrameHeader[bytesMatched]){
			bytesMatched++;
			if(bytesMatched == sizeof(FrameHeader)){
				bytesRead++;
				break;
			}
		}else{
			bytesMatched = 0;
		}
	}

	// Clear buffer if header isn't found
	if(bytesMatched != sizeof(FrameHeader)){
		size_t size = rxBuf.readAvailable();
		rxBuf.clear();
		ESP_LOGD(tag, "Couldn't find frame header. Skipping %zu bytes", size);
		return false;
	}

	// Skip bytes preceding header
	if(bytesRead - bytesMatched != 0){
		rxBuf.skip(bytesRead - bytesMatched);
		ESP_LOGD(tag, "Found header. Skipping %zu bytes", bytesRead - bytesMatched);
	}

	// Needs at least 8 more bytes to continue (frame size + frame size confirmation)
	if(rxBuf.readAvailable() < sizeof(FrameHeader) + sizeof(size_t) * 2){
		return false;
	}

	// Read frame size
	uint8_t frameSizeRaw[4];
	for(int i = 0; i < 4; i++){
		frameSizeRaw[i] = *rxBuf.peek<uint8_t>(sizeof(FrameHeader) + i);
	}
	size_t frameSize = *((size_t*) frameSizeRaw);


	// Clear buffer if frame is malformed
	if(frameSize >= RxBufSize){
		ESP_LOGD(tag, "Frame size yuge: %zu", frameSize);
		rxBuf.clear();
		return false;
	}

	// Read shifted frame size
	uint8_t frameShiftedSizeRaw[4];
	for(int i = 0; i < 4; i++){
		frameShiftedSizeRaw[i] = *rxBuf.peek<uint8_t>(sizeof(FrameHeader) + sizeof(size_t) + i);
	}

	// Clear buffer if shifted size doesn't match size when shifted
	for(int i = 0; i < 4; i++){
		if(frameShiftedSizeRaw[FrameSizeShift[i]] != frameSizeRaw[i]){
			ESP_LOGD(tag, "Frame checksum doesn't match");
			rxBuf.skip(sizeof(FrameHeader));
			return false;
		}
	}

	// Abort if rest of frame is missing
	if(rxBuf.readAvailable() < frameSize + sizeof(FrameHeader) + sizeof(FrameTrailer) + sizeof(size_t) * 2){
		return false;
	}

	// Search for frame trailer
	size_t endOffset = frameSize + sizeof(FrameHeader) + sizeof(size_t) * 2;
	for(bytesMatched = 0; bytesMatched < sizeof(FrameTrailer); bytesMatched++){
		uint8_t byte = *rxBuf.peek<uint8_t>(endOffset + bytesMatched);
		if(byte != FrameTrailer[bytesMatched]) break;
	}

	// Clear whole frame if trailer isn't found at expected offset
	if(bytesMatched != sizeof(FrameTrailer)){
		rxBuf.skip(endOffset + sizeof(FrameTrailer));
		ESP_LOGD(tag, "Trailer missmatch. Clearing %zu bytes", endOffset + sizeof(FrameTrailer));
		return false;
	}

	return true;
}
