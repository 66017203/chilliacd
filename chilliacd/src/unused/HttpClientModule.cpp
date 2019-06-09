#include "HttpClientModule.h"
#include <log4cplus/loggingmacros.h>
#include "json/json.h"
#include "stringHelper.h"
#include "SiaManager.h"
#include "tinyxml2.h"
#include <fstream>
#include "mkpath.h"

class AsynHttpRequest : public HttpRequest{
public:
	AsynHttpRequest(TSiaManager * own, const std::string & sessionId, http_method method, const std::string &uri, const std::string & cmd, const std::string & data = "", int timeout = 0, HttpClientModule * mhttp = nullptr) :
		HttpRequest(sessionId, method, uri, data, timeout), m_pOwn(own), m_cmd(cmd), m_httpMode(mhttp)
	{
	}
	virtual ~AsynHttpRequest(){
		if (this->GetStatus() == 0) {
			Json::Value root;
			Json::Reader read;
			root["status"] =  408;
			root["reason"] = "request timeout";
			root["cmd"] = this->m_cmd;
			Json::FastWriter writer;
			std::string jsonData = writer.write(root);
			
			std::shared_ptr<CMessage> event(new CMessage(0, std::stoul(m_sessionId)));
			m_pOwn->SiaUnpackjson2Message(jsonData, event);
			m_pOwn->SendMessageToMe(std::move(event));
		}
	}

	virtual void onResponse()
	{
		//LOG4CPLUS_DEBUG(log, "Response:" << this->GetResponse());
		if (this->GetStatus()== 0){
			if(this->m_httpMode) this->m_httpMode->DelayRequest(this);
			return;
		}

		Json::Value root;
		Json::Reader read;
		if (!this->m_cmd.empty())
		{
			if (read.parse(this->GetResponse(), root) == false) {
				root["status"] = this->GetStatus();
				root["body"] = this->GetResponse();
			}

			if (root.isObject()) {
				root["cmd"] = this->m_cmd;
			}

			HeaderType headers = this->GetHeader();
			if(!headers["ttsFileName"].empty())
				root["ttsFileName"] = headers["ttsFileName"];

			Json::FastWriter writer;
			std::string jsonData = writer.write(root);
			
			std::shared_ptr<CMessage> event(new CMessage(0, std::stoul(m_sessionId)));
			m_pOwn->SiaUnpackjson2Message(jsonData, event);
			m_pOwn->SendMessageToMe(std::move(event));
		}
		delete this;
	}
private:
	TSiaManager		  * m_pOwn;
	std::string		    m_cmd;
	HttpClientModule *  m_httpMode;
	friend class HttpClientModule;
};

HttpClientModule::HttpClientModule(TSiaManager * own) :m_pOwn(own), m_Idlethread(0)
{
	log = log4cplus::Logger::getInstance("HttpClientMoudle");
}

HttpClientModule::~HttpClientModule()
{
	Stop();

	HttpRequestPtr req = NULL;
	while (m_HttpRequests.Get(req, 1))
		delete req;
}

void HttpClientModule::Start()
{
}

void HttpClientThread::Run()
{
	LOG4CPLUS_DEBUG(log, "Running...");

	m_Base = event_base_new();

	LOG4CPLUS_DEBUG(log, "current method:" << event_base_get_method(m_Base));

	struct event * addRequest = event_new(m_Base, -1, EV_PERSIST, processRequest, this);
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_usec = 50 * 1000;
	event_add(addRequest, &tv);

	event_base_dispatch(m_Base);

	event_free(addRequest);
	event_base_free(m_Base);
	m_Base = nullptr;

	LOG4CPLUS_DEBUG(log, "Stoped");
	log4cplus::threadCleanup();
}

void HttpClientModule::Stop()
{
	for (auto & th : m_threads)
	{
		event_base_loopbreak(th->m_Base);
	}

	for (auto & th : m_threads)
	{
		HttpRequestPtr nullptr_ = nullptr;
		m_HttpRequests.Put(nullptr_);
	}

	for (auto & th : m_threads)
	{
		if (th->m_thread.joinable()) {
			th->m_thread.join();
		}
	}

	m_threads.clear();
}

void HttpClientModule::PutMessage(const std::string & uri, const std::string & sessionid, const std::string & cmd, const std::string & data, HttpRequest::http_method method, const HttpRequest::HeaderType & header, int timeout)
{
	HttpRequestPtr req(new AsynHttpRequest(this->m_pOwn, sessionid, method, uri, cmd, data, timeout, this));
	for (auto it : header){
		req->AddHeader(it.first, it.second);
	}
	m_HttpRequests.Put(req);
	if (m_Idlethread == 0 && m_threads.size() < m_MaxThreadSize){
		m_threads.push_back(std::shared_ptr<HttpClientThread>(new HttpClientThread(this)));
	}
}

void HttpClientThread::processRequest(evutil_socket_t fd, short event, void *arg)
{
	HttpClientThread * This = (HttpClientThread *)arg;
	This->m_Own->m_Idlethread--;
	This->ProcessRequest();
	This->m_Own->m_Idlethread++;
}

void HttpClientThread::ProcessRequest()
{
	HttpRequestPtr req = nullptr;
	while (m_Own->m_HttpRequests.Get(req, 0)){

		LOG4CPLUS_TRACE(log, "Queue size:" << m_Own->m_HttpRequests.size());

		if (req->m_requestTimes > 0){
			LOG4CPLUS_ERROR(req->log, req->m_sessionId << " send http request 1 times error, abandonment.");
			delete req;
			continue;
		}

		if (req->Send(this->m_Base) != 0){
			m_Own->DelayRequest(req);
		}
	}
}

void HttpClientModule::DelayRequest(HttpRequestPtr req)
{
	m_HttpRequests.Put(req);
}

HttpClientThread::HttpClientThread(class HttpClientModule * own) :m_Base(NULL), m_Own(own)
{
	log = log4cplus::Logger::getInstance("HttpClientThread");
	m_thread = std::thread(&HttpClientThread::Run, this);
	m_Own->m_Idlethread++;
}

HttpClientThread::~HttpClientThread()
{
}
