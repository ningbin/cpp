
#include "log_msg.h"
#include "agent_handler.h"
#include "agent_request.h"
#include "agent_response.h"
#include "agent_action.h"

AgentHandler::AgentHandler():_stop(false)
{
	MY_DEBUG(ACE_TEXT("AgentHandler::AgentHandler()\n"));
}

AgentHandler::~AgentHandler()
{
	MY_DEBUG(ACE_TEXT("AgentHandler::~AgentHandler()\n"));
}

int
AgentHandler::open (void *p)
{
    if( super::open(p) == -1 )
    {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("AgentHandler::open")),
                          -1);
    }
             
    ACE_INET_Addr peer_addr;
    peer().get_remote_addr (peer_addr);
    _peer_name = peer_addr.get_host_name();
    _peer_ip =  peer_addr.get_host_addr();
    _peer_port = peer_addr.get_port_number();
    
    MY_NOTICE( ACE_TEXT("connected from %s [%s:%d]\n"),
             _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
    return activate();
}

int
AgentHandler::handle_input (ACE_HANDLE)
{
    ACE_UINT32 header_len = sizeof(ACE_UINT32)+sizeof(ACE_TCHAR);
        
    ACE_Message_Block *mb = new ACE_Message_Block(header_len);
    size_t h_size = peer().recv_n (mb->wr_ptr(),header_len);
    if( h_size <= 0 )
    {
        MY_NOTICE( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                    _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        mb->release();
        return -1;
    }
    else if(h_size!=header_len)
    {
        MY_ERROR(ACE_TEXT("something wrong, Cannot received entire header = %u\n"), h_size);
        mb->release();
        return -1;
    }
    mb->wr_ptr(header_len);
    ACE_UINT32 length;
    ACE_UINT32 net_len;
    
    ACE_OS::memcpy (&net_len, mb->rd_ptr(), sizeof(ACE_UINT32)); 
    mb->rd_ptr(sizeof(ACE_UINT32));
    length = ACE_NTOHL(net_len); 
    
    MY_DEBUG(ACE_TEXT("msg size = %u\n"), length);
    
    if(length==0)
    {
        MY_ERROR(ACE_TEXT("msg size = 0\n"));
        mb->release();
        return 0;
    }
    
    ACE_Message_Block *data = new ACE_Message_Block(length); 
    mb->cont(data); 
    size_t recv_cnt = peer().recv_n (data->wr_ptr(), length);
    data->base()[recv_cnt]='\0';
    data->wr_ptr(recv_cnt+1);
    
    if (recv_cnt > 0)
    {
        MY_INFO( ACE_TEXT ("[%s:%d]>>>>\n%s"),
                          _peer_ip.c_str(), _peer_port,
                          data->rd_ptr());
        putq(mb);
    }
    else if (recv_cnt <= 0 )
    {
        MY_NOTICE( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                    _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        mb->release();
        return -1;
    }
    
    return 0;
}

int
AgentHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask)
{
    MY_DEBUG(ACE_TEXT("AgentHandler::handle_close\n" ));  
    if(!_stop)
    {
        MY_NOTICE( ACE_TEXT ("close connection to %s [%s:%d]\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        
        notify();
        thr_mgr()->wait_task(this);
        reactor()->remove_handler(this, 
                                ACE_Event_Handler::READ_MASK |
                                ACE_Event_Handler::DONT_CALL);
        return super::handle_close();
    }
    return 0;
}

int
AgentHandler::svc( void )
{
    MY_DEBUG(ACE_TEXT("svc\n"));
    ACE_Message_Block *msg;
    while( getq(msg) != -1 )
    {
        ACE_TCHAR type;
        ACE_OS::memcpy (&type, msg->rd_ptr(), sizeof(ACE_TCHAR));
        msg->rd_ptr(sizeof(ACE_TCHAR));

        ACE_Message_Block *data = msg->cont();
        AgentRequest request;
        if( request.fromXml( data->rd_ptr(),type )==-1 )
        {
            msg->release();
            continue;
        }

        AgentResponse response;
        AGENT_ACTION->action(request, response);

        ACE_Message_Block *rep_xml = response.toXml();
        size_t rep_size = rep_xml->length();
        size_t header_size = sizeof(ACE_UINT32)+sizeof(ACE_TCHAR);

        size_t send_cnt = peer().send_n (rep_xml->rd_ptr(), rep_size);        
        if(send_cnt != rep_size)
        {
            MY_ERROR("socket error!! failed to send response!!!");
        }
        else
        {
            MY_INFO( ACE_TEXT ("[%s:%d]<<<<\n%.*C"),
                     _peer_ip.c_str(), _peer_port,
                     static_cast<int> (send_cnt-header_size),
                     rep_xml->rd_ptr()+header_size);
        }

        rep_xml->release();
        msg->release();
    }
    MY_DEBUG(ACE_TEXT("svc end\n"));
    return 0;
}

void AgentHandler::notify( void )
{
	if(!_stop)
    {
        MY_DEBUG(ACE_TEXT("notify()\n"));
        _stop = true;
        msg_queue()->deactivate();
    }
}

