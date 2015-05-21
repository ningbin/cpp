
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "test_handler.h"
#include "test_client.h"
#include "log_msg.h"
#include "tinystr.h" 
#include "tinyxml.h"

#include "ace/High_Res_Timer.h"

TestHandler::TestHandler():
                    _start(false),_stop(true)
{
	MY_DEBUG(ACE_TEXT("TestHandler::TestHandler()\n"));
}

TestHandler::~TestHandler()
{
	MY_DEBUG(ACE_TEXT("TestHandler::~TestHandler()\n"));
}

int
TestHandler::open (void *p)
{
  if( super::open(p) == -1 )
  {
  	ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("TestHandler::open")),
                      -1);
  }
  TestConnector* connector = static_cast<TestConnector*>(p);
  _tid = connector->getTid();

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
TestHandler::handle_input (ACE_HANDLE)
{
  return 0;
}

int
TestHandler::handle_output (ACE_HANDLE)
{
  return 0;
}

int
TestHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask)
{
  MY_DEBUG(ACE_TEXT("TestHandler::handle_close()\n"));

//if( _start && !_stop)
//{
//    reactor()->remove_handler(this,
//                              ACE_Event_Handler::READ_MASK |
//                              ACE_Event_Handler::DONT_CALL);
//    return super::handle_close();
//}
  reactor()->remove_handler(this,
                            ACE_Event_Handler::READ_MASK |
                            ACE_Event_Handler::DONT_CALL);
  delete this;
  return 0;
}

int
TestHandler::svc( void )
{
	MY_DEBUG(ACE_TEXT("svc begin\n"));
	_start = true;
    std::vector<ACE_CString> commands;
    if( load_cmd("tl1.cfg", commands) <0 || commands.size()==0 )
    {
        MY_ERROR("Failed to load TL1 commands !!!\n");
        return -1;
    }
    ACE_TCHAR buf[70000];
    ACE_CString act_user = "ACT-USER:T152:Admin1:100::tellabs1$;";
    size_t p1 = act_user.find(":");
    size_t p2 = act_user.find(":",p1+1);
    if(p1!=ACE_CString::npos && p1!=ACE_CString::npos)
    {
        act_user = act_user.substr(0,p1+1) + _tid + act_user.substr(p2);
    }

    if( send_data(act_user.c_str(), act_user.length())==-1 ||
            recv_data(buf, sizeof buf)==-1 )
    {
        MY_ERROR(ACE_TEXT("ACT-USER Failed!!!\n"));
    }

    MY_DEBUG(ACE_TEXT("Test start\n")); 
    u_long t = 0;
    std::vector<ACE_CString>::const_iterator it=commands.begin();
    ACE_hrtime_t test_start = ACE_OS::gethrtime ();
	while(!_stop && t< 10000)
	{
        if( send_data(it->c_str(), it->length())==-1 ||
            recv_data(buf, sizeof buf)==-1 )
        {
            break;
        }
        ++it;
        ++t;
        if(it==commands.end())
        {
            it=commands.begin();
        }
	}
    ACE_hrtime_t test_end = ACE_OS::gethrtime (); 
    MY_DEBUG(ACE_TEXT("Test end, %u messages sended!\n"), t);
    ACE_TCHAR period[1024]; 
    ACE_OS::sprintf(period, "%llu", test_end-test_start);
//  MY_INFO("Time: %s (ns) \n", period);

    ACE_CString s_time = nsToMs(period);
    MY_INFO("Time: %s (ms) \n", s_time.c_str());
    peer().close_writer();

	MY_DEBUG(ACE_TEXT("svc end\n"));
    
    _stop = true;
    reactor()->end_reactor_event_loop ();
	return 0;
}

ACE_CString TestHandler::nsToMs(const ACE_TCHAR* ns)
{
    ACE_CString s_time = ns;
    ACE_CString s_time2;
    size_t len = s_time.length();
    if(len<=6)
    {
        s_time2 = "0.";        
        for(int j=6-len; j>0; --j)
        {
            s_time2 += "0";
        }
        s_time2 += s_time;
    }
    else
    {
        ACE_CString part1 = s_time.substr(0,len-6);
        s_time2 = "." + s_time.substr(len-6,6);
        size_t len2 = part1.length();
        int k=0;
        for(k=len2; k>3; k=k-3)
        {
            s_time2 = "," + part1.substr(k-3,3)+ s_time2;
        }
        s_time2 = part1.substr(0,k) + s_time2;
    }
    return s_time2;
}

void TestHandler::notify( void )
{
		MY_DEBUG(ACE_TEXT("notify()\n"));
		_stop = true;
//      msg_queue()->deactivate();
//      _cond.signal();
}

int TestHandler::send_data(const ACE_TCHAR* buf, size_t send_size)
{
    size_t send_cnt = 0;
    if ( send_cnt = (peer().send_n (buf, send_size)) != send_size )
    {
        MY_ERROR ( ACE_TEXT ("send msg to %s [%s:%d] failed!!!\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        return -1;
    }
    else
    {
//      MY_DEBUG(">>>>%s\n", buf);
    }
    return 0;
}

int TestHandler::recv_data(ACE_TCHAR* buf, size_t buf_size)
{
  size_t recv_cnt = 0;
  while ( ( recv_cnt = peer().recv (buf, buf_size-1) ) > 0)
  {
//    MY_DEBUG("<<<<%.*C\n", recv_cnt,  buf );
      size_t end = recv_cnt-1;
      while( buf[end]=='\r' || buf[end]=='\n' ) 
      {
          --end;
      }
      if( buf[end]==';')
      {
          buf[recv_cnt] = '\0';
//        MY_DEBUG("<<<<%s\n", buf);
//        MY_DEBUG("<<<<((end))=%.*C\n", 10,  buf+recv_cnt-10 );
          break;
      }
  }
  if (recv_cnt <= 0 )
  {
    MY_NOTICE ( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
    return -1;
  }
  return 0;
}

int TestHandler::load_cmd(const ACE_TCHAR* filename, std::vector<ACE_CString>& commands)
{
    MY_DEBUG("TestHandler::load_cmd begin\n");
    ACE_FILE_Connector connector;
    ACE_FILE_Info fileinfo;
    ACE_FILE_IO file;
    ACE_FILE_Addr fileaddr(filename);

    if (connector.connect (file, fileaddr) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot open file: %s\n"),filename);
        return -1;
    }
    if (file.get_info (&fileinfo) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot get fileinfo of  %s\n"), filename);
        return -1;
    }

    ACE_TCHAR buf[fileinfo.size_];
    u_long rbytes;
    if ((rbytes = file.recv_n (buf, fileinfo.size_ )) != fileinfo.size_)
    {
        MY_ERROR(ACE_TEXT("cannot read file %s correctly!\n"), filename);
        return -1;
    }

    size_t pre = 0;
    size_t eol = 0;
    for(size_t cur=0; cur<fileinfo.size_; cur++)
    {
        if(buf[cur] == '\n' && cur>pre)
        {
            eol = cur;
            while( eol>pre && (buf[eol]==' '||buf[eol]=='\n'))
            {
                --eol;
            }
            while( eol>pre && (buf[pre]==' '||buf[pre]=='\n') )
            {
                ++pre;
            }
            if(eol<pre+1) 
            {
                pre = cur;
                continue;
            }
            else
            {
                ACE_TCHAR *line = ACE::strnnew( static_cast<const char*>(buf)+pre, eol-pre+1) ;
                ACE_CString cmd = line;
                size_t p1 = cmd.find(":");
                size_t p2 = cmd.find(":",p1+1);
                if(p1!=ACE_CString::npos && p1!=ACE_CString::npos)
                {
                    cmd = cmd.substr(0,p1+1) + _tid + cmd.substr(p2);
                }
                commands.push_back(cmd);
                ACE::strdelete( line );
//              MY_DEBUG("CMD=%s\n",cmd.c_str());
            }
            pre = cur;
        }
    }
    MY_DEBUG("TestHandler::load_cmd end\n");
    return 0;
}

