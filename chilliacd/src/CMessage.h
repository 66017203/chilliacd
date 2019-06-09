#pragma once
#include "json/json.h"
#include <string>

#define  Cmd_StartFlow			"cmdstartflow"

#define Evt_Received			"Received"
#define Evt_CallCleared			"CallCleared"
#define Evt_StopService			"stopservice"
#define Evt_StartService		"startservice"
#define Evt_TIMEOUT				"timeout"

// add begin
#define Evt_acdRoute            "acdRoute"
#define Evt_acdRouteEx          "acdRouteEx"
#define Evt_acdReRoute          "acdReRoute"
#define Evt_acdRouteUsed        "acdRouteUsed"
#define Evt_acdRouteEnd         "acdRouteEnd"
// add end

#define Evt_Manager_Stop		"SiaManagerStop"

#define SER_Connection			"connection"
#define SER_CallID				"callid"
#define SER_Called				"called"
#define SER_Caller				"caller"
#define SER_Device				"device"
#define SER_SenderUri			"senderuri"
#define SER_OrgCaller			"orgcaller"
#define SER_OrgCalled			"origcalled"
#define SER_CompanyId			"companyid"
#define SER_BillingRefID		"billid"
#define SER_SessionID			"sessionid"

//const char * CEvt_ConsultationCall = "ConsultationCall";

class CMessage {
public:
	CMessage() {

	};
	CMessage(uint32_t _sender, uint32_t _receiver):sender(_sender),receiver(_receiver) {

	};
	CMessage(uint32_t _sender, uint32_t _receiver, const std::string & _cmd) : cmd(_cmd), sender(_sender), receiver(_receiver) {

	};

	std::string cmd;
	Json::Value jsonData;
	std::string dataString;
	int32_t station = -1;
	uint32_t sender = 0;
	uint32_t receiver = 0;

	CMessage(CMessage&& other)
	{
		this->cmd = std::move(other.cmd);
		this->jsonData = std::move(other.jsonData);
		this->dataString = std::move(other.dataString);
		this->station = other.station;
		this->sender = other.sender;
		this->receiver = other.receiver;
	}

	CMessage& operator = (CMessage&& other) {
		this->cmd = std::move(other.cmd);
		this->jsonData = std::move(other.jsonData);
		this->dataString = std::move(other.dataString);
		this->station = other.station;
		this->sender = other.sender;
		this->receiver = other.receiver;
		return *this;
	}
};
