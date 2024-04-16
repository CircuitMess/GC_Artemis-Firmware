#include "Events.h"
#include <cstring>

std::unordered_map<Facility, std::unordered_set<EventQueue*>> Events::queues;
std::mutex Events::mut;

void Events::listen(Facility facility, EventQueue* queue){
	std::lock_guard lock(mut);

	queues[facility].insert(queue);
}

void Events::unlisten(EventQueue* queue){
	std::lock_guard lock(mut);

	for(auto& pair : queues){
		pair.second.erase(queue);
	}
}

void Events::post(Facility facility, const void* data, size_t size){
	std::unique_lock lock(mut);

	auto pair = queues.find(facility);
	if(pair == queues.end()) return;

	const std::unordered_set<EventQueue*> subs = pair->second;
	lock.unlock();

	for(auto queue : subs){
		void* qData = nullptr;
		if(size != 0){
			qData = malloc(size);
			memcpy(qData, data, size);
		}
		queue->post(facility, qData);
	}
}


EventQueue::EventQueue(size_t count){
	queue = xQueueCreate(count, sizeof(InternalEvent));
}

EventQueue::~EventQueue(){
	reset();
	vQueueDelete(queue);
}

bool EventQueue::get(Event& event, TickType_t timeout){
	InternalEvent internal{};

	if(!xQueueReceive(queue, &internal, timeout)) return false;

	if(internal.killPill) return false;

	event = internal.evt;

	return true;
}

bool EventQueue::post(Facility facility, void* data){
	InternalEvent event = {
			.evt = {
					.facility = facility,
					.data = data
			},
			.killPill = false
	};

	return xQueueSend(queue, &event, 0) == pdTRUE;
}

void EventQueue::reset(){
	while(uxQueueMessagesWaiting(queue) > 0){
		Event evt = {};
		get(evt, 0);
		free(evt.data);
	}
}

void EventQueue::unblock(){
	InternalEvent event = {
			.killPill = true
	};

	xQueueSend(queue, &event, 0);
}
