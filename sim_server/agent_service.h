
#ifndef __AGENT_SERVICE_H_
#define __AGENT_SERVICE_H_

#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Service_Object.h"

#include "agent_handler.h"

typedef ACE_Acceptor<AgentHandler, ACE_SOCK_ACCEPTOR>
  AgentAcceptorT;
  
class AgentAcceptor : public AgentAcceptorT
{	
	public: 
		AgentAcceptor();
		virtual ~AgentAcceptor();
		
		virtual int handle_close (ACE_HANDLE handle,
                            ACE_Reactor_Mask close_mask);
};
  
class AgentService : public ACE_Service_Object
{
	public:
	  virtual int init (int argc, ACE_TCHAR *argv[]);
	  virtual int fini ();
	  virtual int info (ACE_TCHAR **, size_t) const;
	  virtual int suspend ();
	  virtual int resume ();

	private:
	  AgentAcceptor *_acceptor;
	  enum { DEFAULT_PORT = 9411 };
};

#endif /* __AGENT_SERVICE_H_ */
