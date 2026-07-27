#ifndef ARTEMIS_FIRMWARE_PSRAMALLOCATOR_H
#define ARTEMIS_FIRMWARE_PSRAMALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <esp_heap_caps.h>
#include <esp_rom_sys.h>

template<typename T>
class PSRAMAllocator {
public:
	using value_type = T;

	PSRAMAllocator() noexcept = default;

	template<typename U>
	constexpr PSRAMAllocator(const PSRAMAllocator<U>&) noexcept{}

	T* allocate(std::size_t n){
		void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
		if(p == nullptr){
			esp_rom_printf("PSRAMAllocator: out of SPIRAM allocating %u B\n", (unsigned) (n * sizeof(T)));
			abort();
		}
		return static_cast<T*>(p);
	}

	void deallocate(T* p, std::size_t) noexcept{
		heap_caps_free(p);
	}
};

// Stateless: all instances are interchangeable, so container moves/swaps never reallocate.
template<typename A, typename B>
constexpr bool operator==(const PSRAMAllocator<A>&, const PSRAMAllocator<B>&) noexcept{ return true; }

template<typename A, typename B>
constexpr bool operator!=(const PSRAMAllocator<A>&, const PSRAMAllocator<B>&) noexcept{ return false; }

template<typename T>
using PSRAMVector = std::vector<T, PSRAMAllocator<T>>;

// Canonical byte payload buffer kept in PSRAM.
using PSRAMByteBuffer = PSRAMVector<uint8_t>;

// std::string backed by PSRAM. Converts to std::string_view like any basic_string.
using PSRAMString = std::basic_string<char, std::char_traits<char>, PSRAMAllocator<char>>;

#endif //ARTEMIS_FIRMWARE_PSRAMALLOCATOR_H
