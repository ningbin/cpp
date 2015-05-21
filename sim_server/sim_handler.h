
#ifndef __SIM_HANDLER_H_
#define __SIM_HANDLER_H_

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "sim_map.h"
#include "ace/Date_Time.h"

class SimHandler :
  public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
  typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> super;

public:
	SimHandler();                            
	virtual ~SimHandler();

  int open (void * = 0);
  
  // Called when input is available from the client.
  virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE);

  // Called when this handler is removed from the ACE_Reactor.
  virtual int handle_close (ACE_HANDLE handle,
                            ACE_Reactor_Mask close_mask);
	virtual int svc(void);
	void notify( void );
    ACE_CString getTid(){return _tid;}
  
    ACE_UINT64 getRecvMsgCnt(){return _recv_msg_cnt;};
    ACE_UINT64 getSentRplCnt(){return _sent_rpl_cnt;};
    ACE_UINT64 getFailedMsgCnt(){return _failed_msg_cnt;};
    ACE_UINT64 getSentEvtCnt(){return _sent_evt_cnt;};

    ACE_UINT64 getRecvDataSize(){return _recv_data_size;};
    ACE_UINT64 getSentDataSize(){return _sent_data_size;};

    int send_event(const ACE_CString& tid, const ACE_CString& data);

    ACE_CString& getPeerIp(){ return _peer_ip;};
    int getPeerPort(){ return _peer_port;};

  private:
    int processMessage(const ACE_TCHAR* msg);
    int send_data(const ACE_CString& data);
	int send_large_data( const ACE_CString &content, const ACE_CString &header, const size_t cut_size );

    size_t compileEvent(ACE_CString& event_data, const ACE_CString& tid, const ACE_CString& data);
    void GetEventTime(const ACE_Date_Time& dateTime, ACE_CString &todayDate, ACE_CString &todayTime);    
  
    ACE_CString getContentByAid(const ACE_CString &content, const ACE_CString &aid);
    int getValueFromMap(ACE_CString &content, const ACE_CString &tl1cmd, 
                          const ACE_CString &tid, const ACE_CString &aid,
                          const ACE_CString &parameter, SimMap* sim_map);  
    int responseNeNotReachable(const ACE_CString& tid, const ACE_CString& ctag);
    int GetTidIpMap( ACE_CString& data, const ACE_CString& tid);
    int ChangeEONType( ACE_CString& data, const ACE_CString& tid);
                                    
    SimMap* _gne_map;
    ACE_CString _tid;
  	ACE_CString	_peer_name;
  	ACE_CString	_peer_ip;
  	int	_peer_port;
  	bool _stop;
   
    ACE_UINT64 _recv_msg_cnt;
    ACE_UINT64 _sent_rpl_cnt;
    ACE_UINT64 _failed_msg_cnt;
    ACE_UINT64 _sent_evt_cnt;

    ACE_UINT64 _recv_data_size;
    ACE_UINT64 _sent_data_size;

};

#endif /* __SIM_HANDLER_H_ */
