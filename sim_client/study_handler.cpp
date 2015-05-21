
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "study_handler.h"
#include "log_msg.h"
#include "tinystr.h" 
#include "tinyxml.h"

#include "ace/High_Res_Timer.h"

StudyHandler::StudyHandler():_start(false),_stop(true)
{
	MY_DEBUG(ACE_TEXT("StudyHandler::StudyHandler()\n"));
}
StudyHandler::StudyHandler(const ACE_CString& tid):_start(false),_stop(true),_tid(tid)
{
    MY_DEBUG(ACE_TEXT("StudyHandler::StudyHandler() with tid\n"));
}
StudyHandler::~StudyHandler()
{
	MY_DEBUG(ACE_TEXT("StudyHandler::~StudyHandler()\n"));
}

int
StudyHandler::open (void *p)
{
  if( super::open(p) == -1 )
  {
  	ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("StudyHandler::open")),
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
StudyHandler::handle_input (ACE_HANDLE)
{
  return 0;
}

int
StudyHandler::handle_output (ACE_HANDLE)
{
  return 0;
}

//int
//StudyHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask)
//{
//  MY_DEBUG(ACE_TEXT("StudyHandler::handle_close()\n"));
//
//  return super::handle_close();
//}

int
StudyHandler::svc( void )
{
	MY_DEBUG(ACE_TEXT("svc begin\n"));
	_start = true;
    std::vector<ACE_CString> commands;
    if( load_cmd("tl1.cfg", commands) <0 || commands.size()==0 )
    {
        MY_ERROR("Failed to load TL1 commands !!!\n");
        return -1;
    }
    openAutoDiscoveryFile();

    ACE_TCHAR buf[70000];
    ACE_CString act_user = "ACT-USER:";
    act_user += _tid;
    act_user += ":Admin1:100::tellabs1$;";

    if( send_data(act_user.c_str(), act_user.length())==-1 ||
            recv_data(buf, sizeof buf)==-1 )
    {
        MY_ERROR(ACE_TEXT("ACT-USER Failed!!!\n"));
    }

    MY_DEBUG(ACE_TEXT("Autodiscovery start\n")); 
    u_long t = 0;
    std::vector<ACE_CString>::const_iterator it=commands.begin();
    ACE_hrtime_t test_start = ACE_OS::gethrtime ();
	while(!_stop && it!=commands.end() )
	{
        ACE_CString original = *it;
        size_t i = original.find(":");
        size_t j = original.find(":",i+1);
        ACE_CString tl1cmd = original.substr(0,i+1);
        tl1cmd += _tid;
        tl1cmd += original.substr(j);

        if( send_data(tl1cmd.c_str(), tl1cmd.length())==-1 ||
            recv_data(buf, sizeof buf)==-1 )
        {
            break;
        }
        ++it;
        ++t;
	}
    ACE_hrtime_t test_end = ACE_OS::gethrtime (); 
    MY_DEBUG(ACE_TEXT("Autodiscovery end, %u messages sended!\n"), t);
    ACE_TCHAR period[1024]; 
    ACE_OS::sprintf(period, "%llu", test_end-test_start);
//  MY_INFO("Time: %s (ns) \n", period);

    ACE_CString s_time = nsToMs(period);
    MY_INFO("Time: %s (ms) \n", s_time.c_str());
    peer().close_writer();

    closeAutoDiscoveryFile();
    makeTl1Xml();
	MY_DEBUG(ACE_TEXT("svc end\n"));
    
    _stop = true;
    reactor()->end_reactor_event_loop ();
	return 0;
}

ACE_CString StudyHandler::nsToMs(const ACE_TCHAR* ns)
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

void StudyHandler::notify( void )
{
		MY_DEBUG(ACE_TEXT("notify()\n"));
		_stop = true;
//      msg_queue()->deactivate();
//      _cond.signal();
}

int StudyHandler::send_data(const ACE_TCHAR* buf, size_t send_size)
{
    size_t send_cnt = 0;
    size_t wr_cnt = 0;
    if ( send_cnt = (peer().send_n (buf, send_size)) != send_size )
    {
        MY_ERROR ( ACE_TEXT ("send msg to %s [%s:%d] failed!!!\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        return -1;
    }
    else
    {
        MY_DEBUG(">>>>%s\n", buf);
    }

    if ( wr_cnt = (_log_file.send_n (buf, send_size)) != send_size )
    {
        MY_ERROR ( ACE_TEXT ("write autodiscovery log failed!!!\n") );
        return -1;
    }

    return 0;
}

int StudyHandler::recv_data(ACE_TCHAR* buf, size_t buf_size)
{
  size_t recv_cnt = 0;
  size_t wr_cnt = 0;
  while ( ( recv_cnt = peer().recv (buf, buf_size-1) ) > 0)
  {
//    MY_DEBUG("<<<<%.*C\n", recv_cnt,  buf );
      
      if ( wr_cnt = (_log_file.send_n (buf, recv_cnt)) != recv_cnt )
      {
          MY_ERROR ( ACE_TEXT ("write autodiscovery log failed!!!\n") );
          return -1;
      }

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
  if( recv_cnt <= 0 )
  {
    MY_NOTICE ( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
    return -1;
  }

  const ACE_TCHAR* tail = "\n\n";
  if( wr_cnt = (_log_file.send_n (tail, 2)) != 2 )
  {
      MY_ERROR ( ACE_TEXT ("write autodiscovery log failed!!!\n") );
      return -1;
  }

  return 0;
}

int StudyHandler::load_cmd(const ACE_TCHAR* filename, std::vector<ACE_CString>& commands)
{
    MY_DEBUG("StudyHandler::load_cmd begin\n");
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
                ACE::strdelete( line );

                size_t p1 = cmd.find(":");
                size_t p2 = cmd.find(":",p1+1);
                if(p1!=ACE_CString::npos && p1!=ACE_CString::npos)
                {
                    cmd = cmd.substr(0,p1+1) + _tid + cmd.substr(p2);
                }
                commands.push_back(cmd);                
                MY_DEBUG("CMD=%s\n",cmd.c_str());
            }
            pre = cur;
        }
    }
    MY_DEBUG("StudyHandler::load_cmd end\n");
    return 0;
}

int StudyHandler::openAutoDiscoveryFile( void )
{
    MY_DEBUG("StudyHandler::openAutoDiscoveryFile begin\n");
    _autodiscovery_filename = _tid + "_autodiscovery.log";
    ACE_FILE_Connector connector;
    ACE_FILE_Addr fileaddr(_autodiscovery_filename.c_str());

    if (connector.connect (_log_file, fileaddr) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot open file: %s\n"),_autodiscovery_filename.c_str());
        return -1;
    }
    _log_file.truncate (0);

    MY_DEBUG("StudyHandler::openAutoDiscoveryFile end\n");
    return 0;
}
int StudyHandler::closeAutoDiscoveryFile( void )
{
    _log_file.close();

    MY_DEBUG("StudyHandler::closeAutoDiscoveryFile\n");
    return 0;
}

int StudyHandler::makeTl1Xml()
{
    MY_DEBUG("StudyHandler::makeTl1Xml\n");
    ACE_CString tl1cfg = _tid + "_config.xml";
    ACE_CString cmd = "make_xml.pl -f ";
    cmd += _autodiscovery_filename + " -o ";
    cmd += tl1cfg;
    
    MY_DEBUG("system::[%s]\n", cmd.c_str());
    ACE_OS::system(cmd.c_str());
    
    return 0;
}

