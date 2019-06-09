#ifndef __STARTSERVICEMODULE_HEADER
#define __STARTSERVICEMODULE_HEADER
#include "TcpClientModule.h"

class TSiaManager;
class StatMoudle:public TcpClientModule{
public:
	explicit StatMoudle(TSiaManager * own);
	virtual ~StatMoudle();

	virtual void Start(Json::Value address) override;
	virtual void Stop() override;
protected:
	virtual void Login() override;
	virtual void Ping() override;
	virtual void onReceived(const std::string & data) override;

};

#endif
