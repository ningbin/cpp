
#ifndef __AGENT_CLIENT_H_
#define __AGENT_CLIENT_H_

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ace/Service_Object.h"

#include "agent_handler.h"

typedef ACE_Connector <AgentHandler, ACE_SOCK_CONNECTOR> 
	AgentConnectorT;  

class AgentConnector : public AgentConnectorT, public ACE_Task_Base
{
	public:
		AgentConnector();
		virtual ~AgentConnector();
		virtual int handle_close (ACE_HANDLE, ACE_Reactor_Mask);
  	virtual int svc( void );
  	
  	virtual int close (void);
  	virtual int suspend (void);
  	virtual int resume (void);
  	
  	int set_remote_addr(const ACE_INET_Addr& addr){_remote_addr = addr;};
  	ACE_INET_Addr& get_remote_addr(){return _remote_addr;};
  	AgentHandler* getHandler(){return _handler;}
  	void setHandler(AgentHandler* handler){_handler = handler;};
  	
	private:
		ACE_INET_Addr _remote_addr;
		AgentHandler* _handler;
		bool _stop;
		
};
          
class AgentClient : public ACE_Service_Object
{
	public:
	  virtual int init (int argc, ACE_TCHAR *argv[]);
	  virtual int fini ();
	  virtual int info (ACE_TCHAR **, size_t) const;
	  virtual int suspend ();
	  virtual int resume ();

	private:
	  AgentConnector *_connector;
	  enum { DEFAULT_PORT = 1234 };
};

#endif /* __AGENT_CLIENT_H_ */
