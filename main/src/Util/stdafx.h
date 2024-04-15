#ifndef CLOCKSTAR_FIRMWARE_STDAFX_H
#define CLOCKSTAR_FIRMWARE_STDAFX_H

#include <cstdint>
#include <utility>

template<typename T> constexpr
T map(T val, decltype(val) fromLow, decltype(val) fromHigh, decltype(val) toLow, decltype(val) toHigh){
	if(fromLow > fromHigh){
		std::swap(fromLow, fromHigh);
	}

	if(toLow > toHigh){
		std::swap(toLow, toHigh);
	}

	return (val - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

uint64_t millis();
uint64_t micros();

void delayMicros(uint32_t micros);
void delayMillis(uint32_t millis);

void ramReport();

void stackRep(const char* where = nullptr);
void heapRep(const char* where = nullptr);

#endif //CLOCKSTAR_FIRMWARE_STDAFX_H
