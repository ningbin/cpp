
#ifndef __SIM_SERVICE_H_
#define __SIM_SERVICE_H_

#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Service_Object.h"

#include "sim_handler.h"

typedef ACE_Acceptor<SimHandler, ACE_SOCK_ACCEPTOR>
  SimAcceptorT;
  
class SimAcceptor : public SimAcceptorT
{	
	public: 
		SimAcceptor(const ACE_CString& tid, int port);
		virtual ~SimAcceptor();
		
		virtual int handle_close (ACE_HANDLE handle,
                            ACE_Reactor_Mask close_mask);

        ACE_CString& getTid(){ return _tid; };
        int getPort(){return _port;};

    private:
        ACE_CString _tid;
        int _port;
};
  
class SimService : public ACE_Service_Object
{
	public:
	  virtual int init (int argc, ACE_TCHAR *argv[]);
	  virtual int fini ();
	  virtual int info (ACE_TCHAR **, size_t) const;
	  virtual int suspend ();
	  virtual int resume ();

	private:
      

};

#endif /* __SIM_SERVICE_H_ */
