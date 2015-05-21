
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "agent_handler.h"
#include "log_msg.h"
#include "tinystr.h" 
#include "tinyxml.h"
#include "ace/ARGV.h"

AgentHandler::AgentHandler():_start(false),_stop(true),_lock(),_cond(_lock)
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
  
  ACE_INET_Addr local_addr;
  peer().get_local_addr (local_addr);
  _local_name = local_addr.get_host_name();
  _local_ip =  local_addr.get_host_addr();
  _local_port = local_addr.get_port_number();
  
  MY_NOTICE (ACE_TEXT("connected to %s [%s:%d]\n"),
             _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
  MY_NOTICE (ACE_TEXT("local addr = %s [%s:%d]\n"),
             _local_name.c_str(), _local_ip.c_str(), _local_port );
  _stop = false;
  return activate();
}

int
AgentHandler::handle_input (ACE_HANDLE)
{
    ACE_UINT32 header_len = sizeof(ACE_UINT32)+sizeof(ACE_TCHAR);
        
    ACE_Message_Block *mb = new ACE_Message_Block(header_len);
    ssize_t h_size = peer().recv_n (mb->wr_ptr(),header_len);
    if( h_size == 0 )
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
    ACE_TCHAR type;
    
    ACE_OS::memcpy (&net_len, mb->rd_ptr(), sizeof(ACE_UINT32)); 
    mb->rd_ptr(sizeof(ACE_UINT32));
    length = ACE_NTOHL(net_len); 
          
    ACE_OS::memcpy (&type, mb->rd_ptr(), sizeof(ACE_TCHAR));
    mb->rd_ptr(sizeof(ACE_TCHAR));
    
    MY_DEBUG(ACE_TEXT("received header, length=%u, type=%d\n"), length, type);
    
    if(length==0)
    {
        MY_ERROR(ACE_TEXT("receive header, but msg length = 0\n"));
        mb->release();
        return 0;
    }
    
    ACE_Message_Block *data = new ACE_Message_Block(length); 
    mb->cont(data); 
    ssize_t recv_cnt = peer().recv_n (data->wr_ptr(), length);
    data->base()[recv_cnt]='\0';
    data->wr_ptr(recv_cnt+1);
    
    if (recv_cnt > 0)
    {
        MY_INFO( ACE_TEXT ("[%s:%d]>>>>\n%s"),
                          _peer_ip.c_str(), _peer_port,
                          data->rd_ptr());
//      putq(mb);
    }
    else if (recv_cnt <= 0 )
    {
        MY_NOTICE( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                    _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        mb->release();
        return -1;
    }
    mb->release();

    return 0;
}

int
AgentHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask)
{
  MY_DEBUG(ACE_TEXT("AgentHandler::handle_close()\n"));
  if(!_stop || !_start)
  {
  	if(_start)
  	{
  		MY_NOTICE ( ACE_TEXT ("close connection to %s [%s:%d]\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
	  	notify();
        //thr_mgr()->wait_task(this);
	}
  	return super::handle_close();
  }
  delete this;
  return 0;
}

int
AgentHandler::svc( void )
{
	MY_DEBUG(ACE_TEXT("svc begin\n"));
	_start = true;
	_svc_id = ACE_Thread::self();

	while(!_stop)
	{
        const ACE_TCHAR* req_list[] = 
        {
//          "GetSimulatorType",
//          "GetNumberOfNEs",
//          "GetHandledCommandCount",
//          "GetOsStatus",
//          "SendAlarm --switch=ON --NEID=MY_T152 --alarmID=1",
//          "SendAlarm --switch=OFF --NEID=MY_T152 --alarmID=1",
            "StartScheduledJob --",
            0
        };
        int argc = sizeof (req_list)/sizeof (ACE_TCHAR *)-1;
        for(int i=0; i<argc; ++i)
        {         
            ACE_CString req = req_list[i];
            if( !req.empty() && sendRequest(req) == -1 )
            {
                return -1;
            }
            if( !_stop  )
            {     	
                ACE_Time_Value timeout(ACE_OS::gettimeofday());
                timeout += 5;
                MY_DEBUG(ACE_TEXT("begin to wait...\n"));
                _cond.wait(&timeout);
                MY_DEBUG(ACE_TEXT("finish waiting\n"));
            }
        }
	}
	MY_DEBUG(ACE_TEXT("svc end\n"));
	return 0;
}

void AgentHandler::notify( void )
{
		MY_DEBUG(ACE_TEXT("notify()\n"));
		_stop = true;
        msg_queue()->deactivate();
        _cond.signal();
}

int AgentHandler::sendRequest( const ACE_CString& req )
{
    TiXmlDocument doc;  
    TiXmlElement* msg;
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );  
    doc.LinkEndChild( decl );  
 
    TiXmlElement * root = new TiXmlElement( "Message" );  
    doc.LinkEndChild( root );  

    TiXmlElement * head = new TiXmlElement( "Head" );  
    root->LinkEndChild( head );  

    TiXmlElement * version = new TiXmlElement( "Version" );  
    version->LinkEndChild( new TiXmlText( "SMIv1" ));  
    head->LinkEndChild( version );  

    TiXmlElement * time = new TiXmlElement( "Time" ); 
    ACE_TCHAR day_and_time[27];
    ACE::timestamp (day_and_time, sizeof day_and_time);
    time->LinkEndChild( new TiXmlText( day_and_time ));  
    head->LinkEndChild( time );
    
    TiXmlElement * body = new TiXmlElement( "Body" );  
    root->LinkEndChild( body );  
 
    TiXmlElement * request = new TiXmlElement( "Request" );  
    body->LinkEndChild( request );  

    TiXmlElement * body_id = new TiXmlElement( "ID" );  
    request->LinkEndChild( body_id ); 
    ACE_Time_Value now(ACE_OS::gettimeofday ());
    ACE_TCHAR buf[64];
    ACE_OS::sprintf(buf,"%lu%lu",now.sec(),now.usec());
    body_id->LinkEndChild( new TiXmlText( buf ));
    
    ACE_ARGV args(req.c_str());
    ACE_CString cmd = args.argv()[0];
    TiXmlElement * body_action = new TiXmlElement( "Action" );
    body_action->LinkEndChild( new TiXmlText( cmd.c_str()));
    request->LinkEndChild( body_action ); 

    if(args.argc()>1)
    {
        TiXmlElement * body_input = new TiXmlElement( "Input" );
        request->LinkEndChild( body_input );
        
        for(int i=1; i<args.argc(); ++i)
        {
            ACE_CString arg = args.argv()[i];
            size_t pos1 = arg.find("--")+2;
            size_t pos2 = arg.find("=");
            if( pos1!=ACE_CString::npos && pos2!=ACE_CString::npos)
            {
                ACE_CString name = arg.substr(pos1, pos2-pos1);
                ACE_CString value = arg.substr(pos2+1);

                TiXmlElement * element = new TiXmlElement( "Element" );
                body_input->LinkEndChild( element );
                element->SetAttribute("name", name.c_str());
                element->LinkEndChild( new TiXmlText( value.c_str() ));

                MY_DEBUG(ACE_TEXT("Add element, name=%s, value=%s\n"),
                                  name.c_str(), value.c_str());
            }
        }        
    }

    TiXmlPrinter printer;  
    doc.Accept( &printer );

    ACE_UINT32 length = printer.Size();
    MY_DEBUG(ACE_TEXT("Data Size = %u\n"), length);
    
    ACE_UINT32 net_len = ACE_HTONL(length);
    ACE_TCHAR type = 0x05;

//      MY_DEBUG( ACE_TEXT ("<<<<\n%.*C"),
//              static_cast<int> (length),
//              printer.CStr());
    
    ACE_UINT32 data_len = sizeof(ACE_UINT32)+sizeof(ACE_TCHAR)+length;		
    ACE_Message_Block *mb = new ACE_Message_Block(data_len);
    
    ACE_OS::memcpy (mb->wr_ptr(), &net_len, sizeof(ACE_UINT32));
    mb->wr_ptr(sizeof(ACE_UINT32));
    
    ACE_OS::memcpy (mb->wr_ptr(), &type, sizeof(ACE_TCHAR));
    mb->wr_ptr(sizeof(ACE_TCHAR));
    
    ACE_OS::memcpy (mb->wr_ptr(), printer.CStr(), length);
    mb->wr_ptr(length);
    
    if( !_stop )
    {
        size_t sent_cnt = peer().send_n(mb->rd_ptr(), mb->length());
        if (sent_cnt <= 0)
        {
            MY_ERROR(ACE_TEXT("Failed to send\n"));
        }
        else if( sent_cnt == mb->length())
        {
            MY_INFO( ACE_TEXT ("[%s:%d]<<<<\n%s"),
                      _peer_ip.c_str(), _peer_port,
//                    mb->rd_ptr()+sizeof(ACE_UINT32)+sizeof(ACE_TCHAR));
                     printer.CStr());
        }
        else
        {
            MY_ERROR(ACE_TEXT("Something Wrong\n"));
        }
    }
    mb->release();
    
    return 0;
}


