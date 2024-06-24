#include "Comm.h"
#include <RoverStateUtil.h>
#include "Util/Services.h"

Comm::Comm(TCPClient& tcp) : Threaded("Comm", 3 * 1024), tcp(tcp), queue(10){
	Events::listen(Facility::TCP, &queue);
	start();
}

Comm::~Comm(){
	Events::unlisten(&queue);
	queue.unblock();
	stop();
}

void Comm::sendDriveDir(DriveDir dir){
	uint8_t code = CommData::encodeDriveDir(dir);
	ControlPacket packet{ CommType::DriveDir, code };
	sendPacket(packet);
}

void Comm::sendHeadlights(HeadlightsMode headlights) {
	const ControlPacket packet {
		.type = CommType::Headlights,
		.data = (uint8_t)headlights
	};

	sendPacket(packet);
}

void Comm::sendArmPos(ArmPos position) {
	const ControlPacket packet {
			.type = CommType::ArmPosition,
			.data = (uint8_t)position
	};

	sendPacket(packet);
}

void Comm::sendArmPinch(ArmPinch pinch) {
	const ControlPacket packet {
			.type = CommType::ArmPinch,
			.data = (uint8_t)pinch
	};

	sendPacket(packet);
}

void Comm::sendCameraRotation(CameraRotation rotation) {
	const ControlPacket packet {
			.type = CommType::CameraRotation,
			.data = rotation
	};

	sendPacket(packet);
}

void Comm::sendFeedQuality(uint8_t quality){
	const ControlPacket packet {
			.type = CommType::FeedQuality,
			.data = quality
	};

	sendPacket(packet);
}

void Comm::sendModulesEnable(bool enable){
	const ControlPacket packet {
			.type = CommType::ModulesEnable,
			.data = (uint8_t)enable
	};

	sendPacket(packet);
}

void Comm::sendScanningEnable(bool enable){
	const ControlPacket packet {
			.type = CommType::ScanMarkers,
			.data = (uint8_t)enable
	};

	sendPacket(packet);
}

void Comm::sendEmergencyMode(bool state){
	const ControlPacket packet {
			.type = CommType::Emergency,
			.data = (uint8_t)state
	};

	sendPacket(packet);
}

void Comm::sendAudio(bool audio){
	const ControlPacket packet{
			.type = CommType::Audio,
			.data = (uint8_t) audio
	};

	sendPacket(packet);
}

void Comm::sendArmEnabled(bool enabled){
	const ControlPacket packet{
			.type = CommType::ArmControl,
			.data = (uint8_t) enabled
	};

	sendPacket(packet);
}

void Comm::sendControllerBatteryCritical(bool value){
	const ControlPacket packet{
			.type = CommType::ControllerBatteryCritical,
			.data = (uint8_t) value
	};

	sendPacket(packet);
}

void Comm::sendConnectionStrength(ConnectionStrength strength){
	const ControlPacket packet{
			.type = CommType::ConnectionStrength,
			.data = (uint8_t) strength
	};

	sendPacket(packet);
}

void Comm::sendPacket(const ControlPacket& packet){
	if(!tcp.isConnected()) return;

	tcp.write((uint8_t*) &packet, sizeof(ControlPacket));
}

void Comm::loop(){
	bool readOK = false;
	if(tcp.isConnected()){
		ControlPacket packet{};
		readOK = tcp.read(reinterpret_cast<uint8_t*>(&packet.type), sizeof(CommType));

		if(readOK){
			//special read and process case for ModuleData packets
			if(packet.type == CommType::ModuleData){
				ModuleData moduleData{};
				readOK = tcp.read(reinterpret_cast<uint8_t*>(&moduleData), sizeof(ModuleData));
				if(readOK){
					Event e{ CommType::ModuleData, { .moduleData = moduleData }, 0 };
					Events::post(Facility::Comm, e);
				}
			}else{
				readOK = tcp.read(reinterpret_cast<uint8_t*>(&packet.data), sizeof(ControlPacket::data));
				if(readOK){
					Event e = processPacket(packet);
					Events::post(Facility::Comm, e);
				}
			}
		}
	}

	if(!tcp.isConnected() || !readOK){
		::Event event{};
		if(queue.get(event, 1)){
			free(event.data);
		}
	}
}

Comm::Event Comm::processPacket(const ControlPacket& packet) {
	Event event {
		.type = packet.type,
		.raw = packet.data
	};

	switch (packet.type){
        case CommType::Headlights: {
			uint8_t value;
			event.changedOnRover = decodeRoverState(packet.data, value);
			event.headlights = value > 0 ? HeadlightsMode::On : HeadlightsMode::Off;
			break;
		}
		case CommType::Battery: {
			event.batteryPercent = packet.data;
			break;
		}
		case CommType::ArmPosition: {
			event.changedOnRover = decodeRoverState(packet.data, (uint8_t&) event.armPos);
			break;
		}
		case CommType::ArmPinch: {
			event.changedOnRover = decodeRoverState(packet.data, (uint8_t&) event.armPinch);
			break;
		}
		case CommType::CameraRotation: {
			event.changedOnRover = decodeRoverState(packet.data, event.cameraRotation);
			break;
		}
		case CommType::ModulePlug: {
			event.modulePlug = CommData::decodeModulePlug(packet.data);
			break;
		}
		case CommType::NoFeed: {
			event.noFeed = (bool) packet.data;
			break;
		}
		default: {
			break;
		}
	}

	return event;
}