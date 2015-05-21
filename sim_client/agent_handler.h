
#ifndef __AGENT_HANDLER_H_
#define __AGENT_HANDLER_H_

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/Condition_T.h"

class AgentHandler :
  public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
  typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> super;

public:
	AgentHandler();
	virtual ~AgentHandler();

  virtual int open (void * = 0);

  // Called when input is available from the client.
  virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE);

  // Called when this handler is removed from the ACE_Reactor.
  virtual int handle_close (ACE_HANDLE handle,
                            ACE_Reactor_Mask close_mask);
  virtual int svc( void );
  void notify( void );
  
  private:
    int sendRequest(const ACE_CString& req);

  	ACE_CString	_peer_name;
  	ACE_CString	_peer_ip;
  	int	_peer_port;
  	ACE_CString	_local_name;
  	ACE_CString	_local_ip;
  	int	_local_port;
  	bool _stop;
  	bool _start;
  	
  	ACE_thread_t _svc_id;
    ACE_Thread_Mutex _lock;
    ACE_Condition<ACE_Thread_Mutex> _cond;
};

#endif /* __AGENT_HANDLER_H_ */
