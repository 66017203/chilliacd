#ifndef __SIATCPMONITOR_H_
#define __SIATCPMONITOR_H_

#include "TcpClientModule.h"

class TSiaManager;
class SiaTcpMonitor:public TcpClientModule {
public:
	explicit SiaTcpMonitor(TSiaManager * own);
	virtual ~SiaTcpMonitor();
	void Alarm(const std::string & sessionid, const std::string & level, const std::string & content);

protected:
	virtual void Login() override;
	virtual void Ping() override;
	virtual void onReceived(const std::string & data) override;
	void ProcessRecv(const std::string & data);
};

#endif
