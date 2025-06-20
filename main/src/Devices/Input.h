#ifndef CLOCKSTAR_FIRMWARE_INPUT_H
#define CLOCKSTAR_FIRMWARE_INPUT_H

#include "Util/Threaded.h"
#include <unordered_map>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <hal/gpio_types.h>

class Input : public SleepyThreaded {
public:
	Input();
	virtual ~Input();

	enum Button { Up, Down, Select, Alt };
	static const std::unordered_map<Button, const char*> PinLabels;

	struct Data {
		Button btn;
		enum Action { Release, Press } action;
	};

	bool getState(Button btn) const;

private:
	void scan();
	void pressed(Button btn);
	void released(Button btn);

	// button index -> GPIO port
	std::unordered_map<Button, gpio_num_t> PinMap;

	std::unordered_map<Button, bool> btnState;

	enum DbDir { Release, Press, None };
	std::unordered_map<Button, uint64_t> dbTime;
	static constexpr uint64_t SleepTime = 20; // [ms]
	static constexpr uint64_t DebounceTime = 5; // [ms]

	void sleepyLoop() override;

	// Hide public functions
	using Threaded::start;
	using Threaded::stop;

};


#endif //CLOCKSTAR_FIRMWARE_INPUT_H
