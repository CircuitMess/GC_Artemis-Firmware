#include "stdafx.h"
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

uint64_t millis(){
	return micros() / 1000;
}

uint64_t micros(){
	return esp_timer_get_time();
}

void delayMicros(uint32_t micros){
	esp_rom_delay_us(micros);
}

void delayMillis(uint32_t millis){
	vTaskDelay(millis);
}

void ramReport(){
	printf("Free INTERNAL heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
	printf("Free PSRAM heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void stackRep(const char* where){
	if(where){
		printf("%s: ", where);
	}

	auto free = uxTaskGetStackHighWaterMark(nullptr);
	printf("min. free stack: %u B\n", free);
}

void heapRep(const char* where){
	if(where){
		printf("%s:\n", where);
	}

	printf("Free 32b heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL));
	printf("Free 8b  heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
	printf("Free internal DMA heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT|MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT|MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA));
	printf("Free PSRAM  heap: %zu B, largest block %zu B\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
	printf("\n");
}