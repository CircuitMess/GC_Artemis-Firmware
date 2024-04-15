#include "PairService.h"
#include "Util/Services.h"
#include "Util/stdafx.h"

PairService::PairService(WiFiSTA& wifi, TCPClient& tcp) : wifi(wifi), tcp(tcp),
														  thread([this](){ loop(); }, "PairService", 4 * 1024), queue(10){

	Events::listen(Facility::WiFiSTA, &queue);
	thread.start();
	wifi.connect();
}

PairService::~PairService(){
	thread.stop(0);

	if(state == State::Pairing){
		wifi.disconnect();
	}

	while(thread.running()){
		delayMillis(1);
	}

	Events::unlisten(&queue);
}

PairService::State PairService::getState() const{
	return state;
}

void PairService::loop(){
	for(::Event event{}; queue.get(event, 1);){
		if(event.data == nullptr){
			return;
		}

		if(event.facility == Facility::WiFiSTA){
			auto& data = *((WiFiSTA::Event*) event.data);
			processEvent(data);
		}

		free(event.data);
	}

	if(wifi.getState() == WiFiSTA::State::Connecting){
		if(connectStart < 0){
			connectStart = millis();
		}

		if(millis() - connectStart >= AbortTimeout){
			wifi.disconnect();

			if(tcp.isConnected()){
				tcp.disconnect();
			}

			wifi.connect();
		}
	}else{
		connectStart = -1;
	}
}

void PairService::processEvent(const WiFiSTA::Event& event){
	if(event.action == WiFiSTA::Event::Disconnect && tcp.isConnected()){
		tcp.disconnect();
	}

	if(event.action != WiFiSTA::Event::Connect) return;

	if(event.connect.success){
		if(tcp.isConnected()){
			tcp.disconnect();
		}

		bool res = tcp.connect();
		if(!res){
			wifi.disconnect();
		}
		state = res ? State::Success : State::Fail;
	}else{
		if(attempted < ConnectionAttempts){
			++attempted;
			wifi.connect();
			return;
		}

		state = State::Fail;
	}

	thread.stop(0);
}
