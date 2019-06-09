#ifndef __SSMODULE_HEADER
#define __SSMODULE_HEADER
#include "TcpClientModule.h"
//#include "TcpServerModule.h"

class TSiaManager;
class SSMoudle:public TcpClientModule{
//class SSMoudle:public TcpServerModule{
public:
	explicit SSMoudle(TSiaManager * own);
	virtual ~SSMoudle();

	virtual void Start(Json::Value address) override;
	virtual void Stop() override;
protected:
	virtual void Login() override;
	virtual void Ping() override;
	virtual void onReceived(const std::string & data) override;
};

#endif
