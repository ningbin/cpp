
#ifndef __TEST_CLIENT_H_
#define __TEST_CLIENT_H_

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ace/Service_Object.h"

#include "test_handler.h"

typedef ACE_Connector <TestHandler, ACE_SOCK_CONNECTOR> 
	TestConnectorT;  

class TestConnector : public TestConnectorT, public ACE_Task_Base
{
	public:
		TestConnector();
		virtual ~TestConnector();
		virtual int handle_close (ACE_HANDLE, ACE_Reactor_Mask);
      	virtual int svc( void );
      	
      	virtual int close (void);
      	virtual int suspend (void);
      	virtual int resume (void);
      	
      	int set_remote_addr(const ACE_INET_Addr& addr){_remote_addr = addr;};
      	ACE_INET_Addr& get_remote_addr(){return _remote_addr;};
      	TestHandler* getHandler(){return _handler;}
      	void setHandler(TestHandler* handler){_handler = handler;};
      	
        ACE_CString& getTid(){return _tid;};
        void setTid(const ACE_TCHAR* tid){ _tid = tid; };

	private:
		ACE_INET_Addr _remote_addr;
		TestHandler* _handler;
		bool _stop;
		ACE_CString _tid;
};
          
class TestClient : public ACE_Service_Object
{
	public:
	  virtual int init (int argc, ACE_TCHAR *argv[]);
	  virtual int fini ();
	  virtual int info (ACE_TCHAR **, size_t) const;
	  virtual int suspend ();
	  virtual int resume ();

	private:
	  TestConnector *_connector;
	  enum { DEFAULT_PORT = 1234 };
};

#endif /* __TEST_CLIENT_H_ */
