
#ifndef __SIM_HANDLER_H_
#define __SIM_HANDLER_H_

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/Condition_T.h"
#include "ace/FILE_IO.h"
#include <vector>

class StudyHandler :
  public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
  typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> super;

public:
	StudyHandler();
    StudyHandler(const ACE_CString& tid );
	virtual ~StudyHandler();

  virtual int open (void * = 0);

  // Called when input is available from the client.
  virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE);

  // Called when output is possible.
  virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);

//// Called when this handler is removed from the ACE_Reactor.
//virtual int handle_close (ACE_HANDLE handle,
//                          ACE_Reactor_Mask close_mask);
  virtual int svc( void );
  void notify( void );
  
  private:
     int send_data(const ACE_TCHAR* , size_t);
     int recv_data(ACE_TCHAR* , size_t);
     int load_cmd(const ACE_TCHAR* filename, std::vector<ACE_CString>& commands);
     ACE_CString nsToMs(const ACE_TCHAR* ns);

     int openAutoDiscoveryFile( void );
     int closeAutoDiscoveryFile( void );
     int makeTl1Xml();
      
  	ACE_CString	_peer_name;
  	ACE_CString	_peer_ip;
  	int	_peer_port;
  	ACE_CString	_local_name;
  	ACE_CString	_local_ip;
  	int	_local_port;
  	bool _stop;
  	bool _start;
    ACE_CString _tid;
    ACE_CString _autodiscovery_filename;
    ACE_FILE_IO _log_file;

};

#endif /* __SIM_HANDLER_H_ */
