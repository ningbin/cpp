#include "ace/Reactor.h"
#include "ace/Get_Opt.h"
#include "sim_scheduler.h"
#include "log_msg.h"
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "tinyxml.h"
#include "sim_manager.h"
#include "sim_handler.h"

#define DEFAULT_EVENT_CONF "event.xml"

SimScheduler::SimScheduler(const ACE_CString& key, const ACE_CString& args, int index)
        : _key(key),_args(args),_index(index), _interval(1000),_conf(DEFAULT_EVENT_CONF),
          _running(false),_stopped(false),_sent_evt_cnt(0)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::SimScheduler(), index=%d\n"), _index);
}


SimScheduler::~SimScheduler()
{
    MY_DEBUG(ACE_TEXT("SimScheduler::~SimScheduler(), index=%d\n"), _index);
    _event_list.clear();
}

int 
SimScheduler::handle_timeout (const ACE_Time_Value &current_time,
                                    const void *arg)
{
//  MY_DEBUG(ACE_TEXT("SimScheduler::handle_timeout()\n"));    
    _notempty.release();
    return 0;
}

int 
SimScheduler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask m)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::handle_close, index=%d\n"), _index);

    _stopped = true;
    _notempty.release();

    thr_mgr()->wait_task(this);

    delete this;
    return 0;
}

int
SimScheduler::svc( void )
{
    MY_DEBUG(ACE_TEXT("svc, index=%d\n"), _index);

    while( !_stopped )
    {
        _notempty.acquire();
        if(!_stopped)
        {
            this->action();
        }
    }

    MY_DEBUG(ACE_TEXT("svc end, index=%d\n"), _index);
    return 0;
}

int SimScheduler::start(ACE_CString& result)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::start(), index=%d\n"), _index);
    if(_running)
    {
        MY_ERROR("schedule %d is already running!\n", _index);
        result = _args + " is already running";
        return -1;
    }
    
    if( parseArgs()== -1 )
    {
        MY_ERROR("schedule %d args parse error!\n", _index);
        result = _args + " args parse error";
        return -1;
    }
    
    SimNE* ne = SIM_MANAGER->getSimNE(_tid);
    if(ne==NULL)
    {
        MY_ERROR("NE not exists : %s\n", _tid.c_str());
        result = "NE not exists : ";
        result += _tid;
        return -1;
    }

    if( loadEvents()== -1 )
    {
        MY_ERROR("failed to load events, tid=%s, eventid=%s, conf=%s\n", 
                 _tid.c_str(), _event_id.c_str(), _conf.c_str());
        result = "Failed to load events, conf=";
        result += _conf + ", eventid=";
        result += _event_id;
        return -1;
    }
    
    int t_sec = _interval/1000;
    int t_usec = (_interval%1000)*1000;
    ACE_Time_Value interval(t_sec, t_usec);
    ACE_Time_Value initialDelay(interval);

    ACE_Reactor::instance()->schedule_timer(this,
                                            0,
                                            initialDelay,
                                            interval);
    this->activate();
    _running = true;
    return 0;
}

int SimScheduler::parseArgs(void)
{
    ACE_ARGV args(_args.c_str());
    ACE_Get_Opt cmd_opt (args.argc(), args.argv(), ACE_TEXT (":e:c:i:t:"), 0);

    if( cmd_opt.long_option (ACE_TEXT ("eventid"), 'e',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("tid"), 't',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("conf"), 'c',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("interval"), 'i',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 )
    {
        return -1;
    }
    
    ACE_TCHAR c_conf[MAXPATHLEN];
    ACE_TCHAR c_eventid[MAXPATHLEN];
    ACE_TCHAR c_tid[MAXPATHLEN];

    for (int c; (c = cmd_opt()) != EOF; )
    switch (c) {
        case 'e':
          ACE_OS::strsncpy(c_eventid, cmd_opt.opt_arg (), MAXPATHLEN);
          _event_id = c_eventid;
         	break;
        case 'c':
          ACE_OS::strsncpy(c_conf, cmd_opt.opt_arg (), MAXPATHLEN);
          _conf = c_conf;
         	break;
        case 'i':
          _interval = ACE_OS::atoi (cmd_opt.opt_arg ());
         	break;
        case 't':
          ACE_OS::strsncpy(c_tid, cmd_opt.opt_arg (), MAXPATHLEN);
          _tid = c_tid;
         	break;
        case ':':
          MY_ERROR( ACE_TEXT ("-%c requires an argument\n"),cmd_opt.opt_opt() );
          return -1;

        default:
          MY_ERROR( ACE_TEXT ("Parse error!\n"),cmd_opt.opt_opt() );
          return -1;
    }
    
    if(_tid.empty()||_event_id.empty())
    {
        MY_ERROR( "Missing args!\n" );
        return -1;
    }

    return 0;
}

int SimScheduler::stop(ACE_CString& result)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::stop(), index=%d\n"), _index);
    ACE_Reactor::instance()->cancel_timer(this,0);

    _running = false;
    return 0;
}

int SimScheduler::loadEvents(void)
{
    ACE_FILE_Connector connector;
    ACE_FILE_Info fileinfo;
    ACE_FILE_IO file;
    ACE_CString name = _conf;
    ACE_FILE_Addr fileaddr(name.c_str());

    if (connector.connect (file, fileaddr) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot open file: %s\n"),name.c_str());
        return -1;
    }
    else
    {
        MY_DEBUG(ACE_TEXT("open file: %s\n"),name.c_str());
        if (file.get_info (&fileinfo) == -1)
        {
            MY_ERROR(ACE_TEXT("cannot get fileinfo of  %s\n"), name.c_str());
            return -1;
        }
        else
        {
//          MY_DEBUG(ACE_TEXT("file size = %u\n"),fileinfo.size_);
        }
        ACE_TCHAR *buf = new ACE_TCHAR[fileinfo.size_];
        u_long rbytes;
        if ((rbytes = file.recv_n (buf, fileinfo.size_ )) != fileinfo.size_)
        {
            MY_ERROR(ACE_TEXT("cannot read file %s correctly!\n"), name.c_str());
            return -1;
        }
        
        TiXmlDocument doc;
        doc.Parse( buf );
        delete[] buf;
        if (true == doc.Error())
        {
            MY_ERROR(ACE_TEXT("XML parse error!!! file: %s, row: %d, col: %d, error: %s\n"), 
                                name.c_str(), 
                                doc.ErrorRow(),
                                doc.ErrorCol(),
                                doc.ErrorDesc());
            return -1;
        }

        TiXmlElement *element = doc.FirstChildElement( "CONFIGURATION" )->FirstChildElement("Event");
        bool foundEvent = false;
        for(;element;element=element->NextSiblingElement("Event"))
        {
            ACE_CString event_id = element->Attribute("id" );
            if(event_id == _event_id)
            {
                MY_DEBUG("found event id %s\n",_event_id.c_str());
            }
            else
            {
                continue;
            }
            ACE_CString event_text = element->GetText();
            event_text = this->trimText(event_text);
            event_text = "[HEAD]" + event_text;
            event_text += "[END]";
            _event_list.push_back(event_text);

            foundEvent = true;
        }
        if(!foundEvent)
        {
            MY_ERROR("Cannot found events, id=%s\n",_event_id.c_str());
            return -1;
        }
    }
    return 0;
}

ACE_CString SimScheduler::trimText(const ACE_CString& data)
{
    ACE_CString result;
    size_t leng = data.length();
    if( leng <=0 )
        return result;
    ACE_TCHAR *buf = new ACE_TCHAR[leng+1];
    
    size_t begin = 0;
    for( ; begin<leng; begin++)
    {
        if( data[begin]==' ' || data[begin]=='\r' || data[begin]=='\n' )
        {
            continue;
        }
        else
        {
            break;
        }
    }
    int end = leng-1;
    for( ; end>=0; end--)
    {
        if( data[end]==' ' || data[end]=='\r' || data[end]=='\n' )
        {
            continue;
        }
        else
        {
            break;
        }
    }
    size_t i = 0;
    size_t cur = 0;
    for( i=begin; i<=end; ++i)
    {
        buf[cur]=data[i];
        cur++;
    }
    buf[cur] = '\0';

    result = buf;
    delete[] buf;
    MY_DEBUG(ACE_TEXT("result=[%s]\n"), result.c_str());
    return result;
}

int SimScheduler::pause(ACE_CString& result)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::suspend(), index=%d\n"), _index);
    if(_running)
    {
        return ACE_Reactor::instance()->suspend_handler(this);
    }
    return -1;
}

int SimScheduler::proceed(ACE_CString& result)
{
    MY_DEBUG(ACE_TEXT("SimScheduler::resume(), index=%d\n"), _index);
    if(!_running)
    {
        return ACE_Reactor::instance()->resume_handler(this);
    }
    return -1;
}

int SimScheduler::action(void)
{
//  MY_DEBUG(ACE_TEXT("SimScheduler::action(), index=%d\n"), _index);

    SimNE* ne = SIM_MANAGER->getSimNE(_tid);
    if(ne==NULL)
    {
        MY_ERROR("cannot find NE : %s\n",_tid.c_str());
        return 0;
    }
    ACE_CString gne = ne->_gne;
    SimHandlerSet* handlers = SIM_MANAGER->getSimHandlerSet( gne );
    if(!handlers)
    {
        return 0;
    }
    if(handlers->size()>0)
    {
        ACE_CString event_data;
        for(std::list<ACE_CString>::const_iterator
            it = _event_list.begin();
            it!= _event_list.end();
            ++it)
        {
            event_data += *it;
            ++_sent_evt_cnt;
        }
        if(event_data.empty())
        {
            MY_DEBUG("event_data is empty!\n");
            return 0;
        }
        for( SimHandlerSet::iterator
        iter = handlers->begin();
        iter!= handlers->end();
        iter++)
        {
            SimHandler* handler = *iter;
            if(handler)
            {
                handler->send_event(_tid, event_data);
            }
        }
    }
    return 0;
}

SimSchedulerManager::SimSchedulerManager() : _index(0)
{
    MY_DEBUG(ACE_TEXT("SimSchedulerManager::SimSchedulerManager()\n"));
}

SimSchedulerManager::~SimSchedulerManager()
{
    MY_DEBUG(ACE_TEXT("SimSchedulerManager::~SimSchedulerManager()\n"));
    fini();
}
        
int SimSchedulerManager::fini()        
{
    MY_DEBUG(ACE_TEXT("SimSchedulerManager::fini\n"));

    _map.unbind_all();
    _index_map.unbind_all();

    return 0;
}
        
int SimSchedulerManager::stopAllSchedules(ACE_CString& result)
{
    if(_map.current_size()>0)
    {
        for( Scheduler_Map::iterator
        iter = _map.begin ();
        iter!= _map.end ();
        iter++)
        {
            SimScheduler* schedule = (*iter).int_id_;
            if(schedule)
            {
                if( schedule->running() )
                {
                    schedule->stop(result);
                }
            }
        }
    }
    _map.unbind_all();
    _index_map.unbind_all();

    return 0;
}
        
SimScheduler* SimSchedulerManager::getSchedule(int index)
{
    SimScheduler* schedule = 0;
    if(_map.find(index, schedule)==-1)
    {
        MY_INFO("Cannot find schedule %d\n",index);
    }
    return schedule;
}

int SimSchedulerManager::startSchedule( ACE_ARGV& args, int& index, ACE_CString& result )
{
    if(args.argc()<3)
    {
        MY_ERROR("Invalid Command Format!\n");
    }
    ACE_ARGV schedule_args;
    for(int i=1; i<args.argc(); i++) //skip args[0] 'start'
    {
        schedule_args.add(args[i]);
    }

    ACE_CString key;
    if( genKeyFromArgs(schedule_args, key) ==-1 )
    {
        key = schedule_args.buf();
        MY_ERROR("%s parse error!\n",key.c_str());
        result = key + " parse error!";
        return -1;
    }

    if(_index_map.find( key,index )!=-1)
    {
        MY_INFO("%s already exists!\n",key.c_str());
        result = key + " already exists";
        return -1;
    }

    index = this->getIndex();
    SimScheduler* schedule = 0;
    if(_map.find( index,schedule )!=-1)
    {
        MY_ERROR("schedule %d already exists!\n",index);
        result = key + " already exists";
        return -1;
    }
    ACE_NEW_RETURN(schedule, SimScheduler(key, schedule_args.buf(),index), -1);
    if ( schedule->start(result) == -1 )
    {
        MY_ERROR("schedule %d failed to start!\n", index);
        delete schedule;
        schedule = 0;
        return -1;
    }
    _index_map.bind(key, index);
    _map.bind(index, schedule);
    return 0;
}

int SimSchedulerManager::stopSchedule( ACE_ARGV& args, ACE_CString& result )
{
    if(args.argc()<3)
    {
        MY_ERROR("Invalid Command Format!\n");
    }
    ACE_CString s_index = args.argv()[2];
    if(s_index == "all")
    {
        return this->stopAllSchedules(result);
    }
    int index = ACE_OS::atoi(s_index.c_str());

    SimScheduler* schedule = 0;
    if(_map.find(index, schedule )==-1)
    {
        MY_DEBUG("schedule %d doesn't exist!\n",index);
        result = "schedule " + s_index + " not exists";
        return -1;
    }
    ACE_CString key = schedule->getKey();
    if(schedule)
    {
        if( schedule->running() )
        {
            schedule->stop(result);
        }
    }
    _map.unbind(index);
    _index_map.unbind(key);
    return 0;
}

int SimSchedulerManager::getIndexFromKey(const ACE_CString& key)
{
    int index = -1;
    if(_index_map.find( key,index )==-1)
    {
        MY_WARNING("schedule %s not exists!\n",key.c_str());
    }
    return index;
}

int SimSchedulerManager::getIndex()
{
    int index=0;

    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, _lock, -1);
    index = ++_index;

    return index;
}

int SimSchedulerManager::genKeyFromArgs(ACE_ARGV& args, ACE_CString& key )
{
    ACE_Get_Opt cmd_opt (args.argc(), args.argv(), ACE_TEXT (":e:c:i:t:"), 0);

    if( cmd_opt.long_option (ACE_TEXT ("eventid"), 'e',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("tid"), 't',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("conf"), 'c',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 ||
    cmd_opt.long_option (ACE_TEXT ("interval"), 'i',
                       ACE_Get_Opt::ARG_REQUIRED) != 0 )
    {
        return -1;
    }
    
    ACE_TCHAR c_conf[MAXPATHLEN]={0};
    ACE_TCHAR c_eventid[MAXPATHLEN]={0};
    ACE_TCHAR c_tid[MAXPATHLEN]={0};
    int interval=0;

    for (int c; (c = cmd_opt()) != EOF; )
    switch (c) {
        case 'e':
          ACE_OS::strsncpy(c_eventid, cmd_opt.opt_arg (), MAXPATHLEN);
         	break;
        case 'c':
          ACE_OS::strsncpy(c_conf, cmd_opt.opt_arg (), MAXPATHLEN);
         	break;
        case 'i':
          interval = ACE_OS::atoi (cmd_opt.opt_arg ());
         	break;
        case 't':
          ACE_OS::strsncpy(c_tid, cmd_opt.opt_arg (), MAXPATHLEN);
         	break;
        case ':':
          MY_ERROR( ACE_TEXT ("-%c requires an argument\n"),cmd_opt.opt_opt() );
          return -1;

        default:
          MY_ERROR( ACE_TEXT ("Parse error!\n"),cmd_opt.opt_opt() );
          return -1;
    }
    if(ACE_OS::strlen(c_tid)==0 || ACE_OS::strlen(c_eventid)==0 )
    {
        MY_ERROR( ACE_TEXT ("Missing arguments!\n") );
        return -1;
    }
    if(ACE_OS::strlen(c_conf)==0)
    {
        ACE_OS::strcpy(c_conf, DEFAULT_EVENT_CONF);
    }
    key = "--tid=";
    key += c_tid;
    key += " --eventid=";
    key += c_eventid;
    key += " --conf=";
    key += c_conf;

    return 0;
}

