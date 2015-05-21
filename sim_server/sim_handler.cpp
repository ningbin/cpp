
#include "sim_handler.h"
#include "log_msg.h"
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "sim_service.h"
#include "sim_manager.h"
#include "sim_monitor.h"
#include "common_utility.h"

const ACE_UINT16 max_tl1_len = 1024;

SimHandler::SimHandler():_stop(false),_recv_msg_cnt(0),_sent_rpl_cnt(0),
                        _sent_evt_cnt(0),_recv_data_size(0),_sent_data_size(0),
                        _failed_msg_cnt(0),_gne_map(0)
{
	MY_DEBUG(ACE_TEXT("SimHandler::SimHandler()\n"));
}

SimHandler::~SimHandler()
{
	MY_DEBUG(ACE_TEXT("SimHandler::~SimHandler()\n"));
}

int
SimHandler::open (void *p)
{
    if( super::open(p) == -1 )
    {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("SimHandler::open")),
                          -1);
    }
    
    SimAcceptor* accptor = static_cast<SimAcceptor*>(p);
    _tid = accptor->getTid();
    SIM_MANAGER->addSimHandler(_tid, this);
    SIM_MONITOR->addMonitorSet(this);
             
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
SimHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask)
{
    MY_DEBUG(ACE_TEXT("SimHandler::handle_close\n" ));  
    if(!_stop)
    {
        MY_NOTICE( ACE_TEXT ("close connection to %s [%s:%d]\n"),
                _peer_name.c_str(), _peer_ip.c_str(), _peer_port );
        
        notify();
        thr_mgr()->wait_task(this);
        reactor()->remove_handler(this, 
                                ACE_Event_Handler::READ_MASK |
                                ACE_Event_Handler::DONT_CALL);
        SIM_MANAGER->removeSimHandler(_tid, this);
        return super::handle_close();
    }
    return 0;
}

int
SimHandler::handle_input (ACE_HANDLE)
{
    const size_t buff_size = max_tl1_len;
    ACE_Message_Block *data = new ACE_Message_Block(buff_size);
    size_t recv_cnt = peer().recv (data->wr_ptr(), buff_size);
    
    if (recv_cnt > 0)
    {
        data->base()[recv_cnt] = '\0';
        data->wr_ptr(recv_cnt+1);
//      MY_INFO( ACE_TEXT ("[%s:%d]>>\n%s\n"),
//                          _peer_ip.c_str(), _peer_port,
//                          data->rd_ptr());
        putq(data);
        _recv_data_size += recv_cnt;
    }
    else if (recv_cnt <= 0 )
    {
        MY_NOTICE( ACE_TEXT ("disconnected by %s [%s:%d]\n"),
                    _peer_name.c_str(), _peer_ip.c_str(), _peer_port );

        data->release();
        return -1;
    }

    return 0;
}

int
SimHandler::svc( void )
{
    MY_DEBUG(ACE_TEXT("svc\n"));

    ACE_Message_Block *msg;
    _gne_map = SIM_MANAGER->getSimNE(_tid)->getSimMap();

    if(_gne_map==NULL)
    {
       MY_ERROR("cannot find sim_map, tid=%s\n", _tid.c_str());
       return -1;
    } 
    
    while( getq(msg) != -1 )
    {
        ACE_TCHAR *begin = msg->rd_ptr();
        ACE_TCHAR *end = msg->wr_ptr();
        for(ACE_TCHAR* c=begin; c!=end; ++c)
        {
            if( *c == ';')
            {
                ACE_TCHAR buf[max_tl1_len];
                ACE_OS::strncpy (buf, begin, c-begin+1);
                buf[c-begin+1] = '\0';
                ++_recv_msg_cnt;
                if( processMessage( buf )==-1 )
                {
                    msg->release();
                    return -1;
                }
                ++_sent_rpl_cnt;
                begin = c+1;
            }
        }
        msg->release();
    }
    MY_DEBUG(ACE_TEXT("svc end\n"));
}

int SimHandler::processMessage(const ACE_TCHAR* msg)
{
//  MY_DEBUG(ACE_TEXT("TL1 = %s\n"), msg);

    std::vector<ACE_CString> tokens;
    CommonUtility::tokenize( tokens, msg, ':', ';' );
    if(tokens.size()<4) 
    {
        MY_ERROR("Invalid TL1 command: %s\n",msg);
        ++_failed_msg_cnt;
        return -1;
    }

    ACE_CString cmd = tokens[0];
    ACE_CString tid = tokens[1];
    ACE_CString aid = tokens[2];
    ACE_CString ctag = tokens[3];
    ACE_CString parameter;
    if(tokens.size()>6)
    {
        parameter = tokens[6];
//      MY_DEBUG("parameter=%s\n",parameter.c_str());
    }
    ACE_CString reply_body;

    if(cmd == "RTRV-TIDIPMAP")
    {
        if( GetTidIpMap( reply_body,tid)==-1 )
        {
            MY_ERROR("Cannot get TIDIPMAP, tid=%s\n",tid.c_str());
            ++_failed_msg_cnt;
        } 
    }
    else
    {
        SimMap* sim_map = 0;
        if( tid == _tid ) //gne
        {
            sim_map = _gne_map;
        }
        else //rne
        {
            bool foundRne = false;
            SimNE* rne = SIM_MANAGER->getSimNE(tid);
            if( rne && rne->_gne == _tid )
            {
                sim_map = SIM_MANAGER->getSimNE(tid)->getSimMap();
                foundRne = true;
            }
            if(!foundRne)
            {
                responseNeNotReachable(tid,ctag);
                return 0;
            }
        }
        if( getValueFromMap(reply_body,cmd,tid,aid,parameter,sim_map)==-1 )
        {
            MY_WARNING("No mapping, TL1=%s\n",msg);
//          MY_WARNING("No mapping, cmd=%s\n",cmd.c_str());
            ++_failed_msg_cnt;
        }
    }
    ACE_CString header("    ");
    header += tid + " ";
    
    ACE_TCHAR c_time[27];
    ACE::timestamp (c_time, sizeof c_time);
    header += c_time;
    header += "\r\nM ";
    header += ctag + " COMPLD\r\n";
    
    const size_t cut_size = 5000;
    if( cut_size < reply_body.length())
    {
        send_large_data(reply_body, header, cut_size);
    }
    else
    {
        ACE_CString reply = header + reply_body + "\r\n;\r\n";
        send_data( reply );
    }
    return 0;
}

int SimHandler::GetTidIpMap( ACE_CString& data, const ACE_CString& tid)
{
    SimNE* ne = SIM_MANAGER->getSimNE(tid);
    if( ne )
    {
        if( ne->_type=="gne" || ne->_type=="GNE")
        {
            data = ne->getTidIpMap();
        }
        return 0;
    }
    return -1;
}

int SimHandler::responseNeNotReachable(const ACE_CString& tid, const ACE_CString& ctag)
{
    ACE_CString header("    ");
    header += tid + " ";
    ACE_TCHAR c_time[27];
    ACE::timestamp (c_time, sizeof c_time);
    header += c_time;
    header += "\r\nM ";
    header += ctag + " DENY\r\n";

    const ACE_TCHAR* content = 
        "    SRTO\n"
        "    /* Unable to reach the target NE.. */\n";
    
    ACE_CString reply = header + content + "\r\n;\r\n";

    return send_data( reply );
}

int SimHandler::getValueFromMap(ACE_CString &content, const ACE_CString &tl1cmd, 
                                  const ACE_CString &tid, const ACE_CString &aid,
                                  const ACE_CString &parameter, SimMap* sim_map)                                          
{
    if( parameter.empty() )
    {
        if( sim_map->getValue(tl1cmd, content)==-1 )
        {
//          MY_WARNING("No value, key=%s\n",tl1cmd.c_str());
            return -1;
        }
        if( !aid.empty() && aid.substr(0,3)!="ALL" )
        {
            content = getContentByAid(content, aid);
        }
        if( tl1cmd == "RTRV-NET")
        {
            ChangeEONType(content, tid);
        }
    }
    else
    {
        size_t len = parameter.length();
        ACE_TCHAR* tmp = ACE::strnew(parameter.c_str());
        
        for(int i=0; i<len; i++)
        {
            if(parameter[i]=='='||parameter[i]==',')
            {
                tmp[i]=':';
            }
            else
            {
                tmp[i]=parameter[i];
            }
        }
        tmp[len] = '\0';
        ACE_CString cmd = tl1cmd;
        cmd += ":" + aid + ":";\
        cmd += tmp;
        ACE::strdelete(tmp);

        if( sim_map->getValue(cmd, content)==-1 )
        {
//          MY_WARNING("No value, key=%s\n",cmd.c_str());
            return -1;
        }
    }
    return 0;
}

int SimHandler::ChangeEONType( ACE_CString& data, const ACE_CString& tid)
{
    ACE_CString ne_type = SIM_MANAGER->getSimNE(tid)->_type;
    ACE_CString eon_type;
    if(ne_type=="rne")
    {
        eon_type = "PRNE";
    }
    else
    {
        eon_type = "PGNE-1";
    }
    ACE_CString tmp = data;
    size_t begin = 0;
    size_t end = 0;

    if((begin = tmp.find("EONTYPE=")) != ACE_CString::npos && 
       (end = tmp.find(",")) != ACE_CString::npos )
    {
        begin += ACE_OS::strlen("EONTYPE=");
        tmp = tmp.substr(0,begin) + eon_type + tmp.substr(end);
    }
    data = tmp.c_str();
    return 0;
}

ACE_CString SimHandler::getContentByAid(const ACE_CString &content, const ACE_CString &aid)
{
    ACE_CString tag = "\"" + aid + ":";
    size_t i = content.find(tag.c_str());
    size_t j = content.find("\"\n", i);
    size_t k = content.find("\"\r\n", i);
    j = j<k?j:k;
    ACE_CString result = content.substr(i, j-i+1);    

    return result;
}

int SimHandler::send_data(const ACE_CString& data)
{
    ssize_t send_cnt = peer().send_n (data.c_str(), data.length());
    if(send_cnt <= 0 )
    {
        MY_ERROR("Send Error!");
        return -1;
    }
    else if(send_cnt == data.length())
    {
//      MY_DEBUG( ACE_TEXT ("[%s:%d]<<\n%s\n"),
//                    _peer_ip.c_str(), _peer_port,
//                    data.c_str());
//      MY_DEBUG( ACE_TEXT ( "sended msg size=%u\n"), send_cnt);
        _sent_data_size += send_cnt;
    }
    else
    {
        MY_ERROR("Something wrong!!");
        return -1;
    }
    return 0;   
}

void SimHandler::notify( void )
{
	if(!_stop)
    {
        MY_DEBUG(ACE_TEXT("notify()\n"));
        _stop = true;
        msg_queue()->deactivate();
    }
}

int SimHandler::send_large_data( const ACE_CString &content, const ACE_CString &header, const size_t cut_size )
{
    size_t len = content.length();

    ACE_CString large_data;
    ACE_CString part;
    const ACE_TCHAR* continuation = "\r\n>\r\n";
    const ACE_TCHAR* termination = "\r\n;\r\n";
    size_t pre = 0;
    size_t cur = cut_size;

    while( cur<len )
    {
        if(cur>=len-1)
        break;
        
        while( content[cur]!='\n' )
        {
            --cur;
        }
        part = header;
        part += content.substr(pre, cur-pre+1);
        part += continuation;
        large_data += part;

        pre = cur;
        cur = cur + cut_size;
    }
    if(cur>=len-1)
    {
        part = header;
        part += content.substr(pre);
        part += termination;
        large_data += part;
    }

    return send_data( large_data );

}

int SimHandler::send_event(const ACE_CString& tid, const ACE_CString& data)
{
    ACE_CString event_data;
    size_t evt_cnt = this->compileEvent(event_data, tid, data);
    if( send_data(event_data) ==0 );
    {
        _sent_evt_cnt += evt_cnt;
//      MY_DEBUG( ACE_TEXT ("[%s:%d]<<\n%s\n"),
//                _peer_ip.c_str(), _peer_port,
//                response.c_str());
        return 0;
    }
    return -1;
}

size_t SimHandler::compileEvent(ACE_CString& event_data, const ACE_CString& tid, const ACE_CString& data)
{
    size_t event_cnt = 0;
    ACE_CString response = data;
    ACE_CString header = "    ";
    ACE_CString dateString;
    ACE_CString timeString;
    
    ACE_Date_Time now(ACE_OS::gettimeofday());
    this->GetEventTime(now, dateString, timeString);
    ACE_TCHAR c_time[27];
    ACE::timestamp (c_time, sizeof c_time);

    header += tid.c_str();
    header += " ";
    header += + c_time;
    header += "\r\n";
    
    size_t pos = 0;
    while ((pos = response.find("[HEAD]", pos)) != ACE_CString::npos)
    {
        response = response.substr(0,pos) + header + response.substr(pos+6);
    }

    pos = 0;
    while ((pos = response.find("[DATE]", pos)) != ACE_CString::npos)
    {
        response = response.substr(0,pos) + dateString.substr (3, 5) + response.substr(pos+6);
    }

    pos = 0;
    while ((pos = response.find("[TIME]", pos)) != ACE_CString::npos)
    {
        response = response.substr(0,pos) + timeString + response.substr(pos+6);
    }

    pos = 0;
    while ((pos = response.find("[END]", pos)) != ACE_CString::npos)
    {
        response = response.substr(0,pos) + "\r\n;\r\n" + response.substr(pos+5);
        ++event_cnt;
    }

    event_data = response.c_str();
    return event_cnt;
}


void SimHandler::GetEventTime(const ACE_Date_Time& dateTime, ACE_CString &todayDate, ACE_CString &todayTime)
{
	char month[3], day[3], year[5];
    char hour[3], minute[3], second[3];
	
	ACE_OS::sprintf(year, "%d", dateTime.year());	
    if (dateTime.year() >= 0 && dateTime.year() < 10)
    {
        todayDate = todayDate + "0";
    }

    todayDate = todayDate + year + "-";
    todayDate = todayDate.substr(2);

    ACE_OS::sprintf(month, "%d", dateTime.month());
    if (dateTime.month() >= 0 && dateTime.month() < 10)
        todayDate = todayDate + "0";
    todayDate = todayDate + month + "-";


    ACE_OS::sprintf(day, "%d", dateTime.day());
    if (dateTime.day() >= 0 && dateTime.day() < 10)
        todayDate = todayDate + "0";
    todayDate = todayDate + day;

    ACE_OS::sprintf(hour, "%d", dateTime.hour());
    if (dateTime.hour() >= 0 && dateTime.hour() < 10)
        todayTime = todayTime + "0";
    todayTime = todayTime + hour + "-";

    ACE_OS::sprintf(minute, "%d", dateTime.minute());
    if (dateTime.minute() >= 0 && dateTime.minute() < 10)
        todayTime = todayTime + "0";
    todayTime = todayTime + minute + "-";

    ACE_OS::sprintf(second, "%d", dateTime.second());
    if (dateTime.second() >= 0 && dateTime.second() < 10)
        todayTime = todayTime + "0";
    todayTime = todayTime + second;
	
}

