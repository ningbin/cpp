
#ifndef __SIM_CLIENT_H_
#define __SIM_CLIENT_H_

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ace/Service_Object.h"

#include "study_handler.h"

typedef ACE_Connector <StudyHandler, ACE_SOCK_CONNECTOR> 
	StudyConnectorT;  

class StudyConnector : public StudyConnectorT, public ACE_Task_Base
{
	public:
		StudyConnector();
		virtual ~StudyConnector();
		virtual int handle_close (ACE_HANDLE, ACE_Reactor_Mask);
      	virtual int svc( void );
      	
      	virtual int close (void);
      	virtual int suspend (void);
      	virtual int resume (void);
      	
      	int set_remote_addr(const ACE_INET_Addr& addr){_remote_addr = addr;};
      	ACE_INET_Addr& get_remote_addr(){return _remote_addr;};
      	StudyHandler* getHandler(){return _handler;}
      	void setHandler(StudyHandler* handler){_handler = handler;};
      	void setTid(const ACE_CString& tid){_tid = tid;};
        ACE_CString getTid(){return _tid;};

    private:
		ACE_INET_Addr _remote_addr;
		StudyHandler* _handler;
		bool _stop;
        ACE_CString _tid;
		
};
          
class StudyClient : public ACE_Service_Object
{
	public:
	  virtual int init (int argc, ACE_TCHAR *argv[]);
	  virtual int fini ();
	  virtual int info (ACE_TCHAR **, size_t) const;
	  virtual int suspend ();
	  virtual int resume ();

	private:
	  StudyConnector *_connector;
	  enum { DEFAULT_PORT = 1234 };
};

#endif /* __SIM_CLIENT_H_ */
