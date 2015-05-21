
#include "log_msg.h"
#include "ace/Monitor_Point_Registry.h"
#include "ace/Monitor_Admin_Manager.h"

#include "sim_monitor.h"
#include "sim_manager.h"
#include "common_utility.h"

MonitorPoint::MonitorPoint() : 
            _current(0),_total(0),_base(0)
{}

MonitorPoint::~MonitorPoint()
{}

MonitorSet::MonitorSet(const ACE_CString& key, SimHandler* handler) : 
                    _key(key), _handler(handler)
{   
    MY_DEBUG(ACE_TEXT("MonitorSet::MonitorSet(), key=%s\n"), _key.c_str());
}

MonitorSet::~MonitorSet()
{
    MY_DEBUG(ACE_TEXT("MonitorSet::~MonitorSet(), key=%s\n"), _key.c_str());
}

void MonitorSet::reset()
{
    _recvMsgCnt.reset();
    _sentRplCnt.reset();
    _failedMsgCnt.reset();
    _sentEvtCnt.reset();
    _recvDataSize.reset();
    _sentDataSize.reset();
}

void MonitorSet::update()
{
    if( _handler && SIM_MANAGER->findSimHandler(_key, _handler)!=-1)
    {
        _recvMsgCnt.update(_handler->getRecvMsgCnt());
        _sentRplCnt.update(_handler->getSentRplCnt());
        _failedMsgCnt.update(_handler->getFailedMsgCnt());
        _sentEvtCnt.update(_handler->getSentEvtCnt());
        _recvDataSize.update(_handler->getRecvDataSize());
        _sentDataSize.update(_handler->getSentDataSize());
    }
}

SimMonitor::SimMonitor()
{
    MY_DEBUG(ACE_TEXT("SimMonitor::SimMonitor()\n"));
    init();
}

SimMonitor::~SimMonitor()
{
    MY_DEBUG(ACE_TEXT("SimMonitor::~SimMonitor()\n"));
    fini();
}

int SimMonitor::init( void )
{
    MY_DEBUG(ACE_TEXT("SimMonitor::init\n"));
    
    _cpu_load_monitor = create_os_monitor<CPU_LOAD_MONITOR>();
    _memory_usage_monitor = create_os_monitor<MEMORY_USAGE_MONITOR>();
    _num_threads_monitor = create_os_monitor<NUM_THREADS_MONITOR>();    
        
    return 0;
}

int SimMonitor::fini( void )
{
    MY_DEBUG(ACE_TEXT("SimMonitor::fini\n"));
    
    _cpu_load_monitor->remove_ref ();
    _memory_usage_monitor->remove_ref ();
    _num_threads_monitor->remove_ref ();    
    
    deleteAllMonitorSets();

    return 0;
}

double SimMonitor::getOsCpuUsage()
{
    Monitor_Control_Types::Data data (_cpu_load_monitor->type ());
    _cpu_load_monitor->update();
    _cpu_load_monitor->retrieve (data);
//  MY_DEBUG("CPU Load : %.2f %%\n",data.value_);

    return data.value_;
}

double SimMonitor::getOsMemUsage()
{
    Monitor_Control_Types::Data data (_memory_usage_monitor->type ());
    _memory_usage_monitor->update();
    _memory_usage_monitor->retrieve (data);
//  MY_DEBUG("Memory Usage: %.2f %%\n",data.value_);

    return data.value_;
}

int SimMonitor::getProcNumThreads()
{
    Monitor_Control_Types::Data data (_num_threads_monitor->type ());
    _num_threads_monitor->update();
    _num_threads_monitor->retrieve (data);
    int n = (int)data.value_;
//  MY_DEBUG("Number Of Threads: %d\n",n);

    return n;
}

int SimMonitor::getOsUname(ACE_CString& uname)
{
    ACE_CString cmd = "uname";     
    uname = this->getExecOutput(cmd.c_str());

    MY_DEBUG("cmd = %s\n",cmd.c_str());
    MY_DEBUG("output = %s\n",uname.c_str()); 
    return 0;
}

int SimMonitor::getProcUsage(ACE_CString& pcpu, ACE_CString& pmem,
                             ACE_CString& vsz, ACE_CString& rss)
{
    
    ACE_CString pid = CommonUtility::str_int(ACE_OS::getpid()); 
    
    ACE_CString cmd = "ps -p [PID] u | grep -v COMMAND | awk \'{print $3,$4,$5,$6}\'";
    size_t pos = ACE_CString::npos;
    while ((pos = cmd.find("[PID]")) != ACE_CString::npos)
    {
        cmd = cmd.substr(0, pos) + pid + cmd.substr(pos+5);
    }
    ACE_CString usage = this->getExecOutput(cmd.c_str());

//  MY_DEBUG("cmd = %s\n",cmd.c_str());
//  MY_DEBUG("output = %s\n",usage.c_str());

    if ((pos = usage.find(" ")) != ACE_CString::npos)
    {
        pcpu = usage.substr(0,pos);
        usage = usage.substr(pos+1);
        pcpu += "%";
    }
    if ((pos = usage.find(" ")) != ACE_CString::npos)
    {
        pmem = usage.substr(0,pos);
        usage = usage.substr(pos+1);
        pmem += "%";
    }
    if ((pos = usage.find(" ")) != ACE_CString::npos)
    {
        vsz = usage.substr(0,pos);
        usage = usage.substr(pos+1);
    }
    rss = usage;
    return 0;
}

ACE_CString SimMonitor::getExecOutput(const ACE_TCHAR* cmd)
{
    ACE_CString result;
    FILE *read_fp;
    ACE_TCHAR buffer[BUFSIZ+1];
    size_t chars_read;

    ACE_OS::memset(buffer, 0 ,sizeof buffer);
    read_fp = ::popen(cmd, "r");
    
    if(read_fp != NULL)
    {
        chars_read = ::fread(buffer, sizeof(ACE_TCHAR), BUFSIZ, read_fp);
        while(chars_read>0)
        {
            buffer[chars_read-1]='\0';
            result += buffer;
            chars_read = ::fread(buffer, sizeof(ACE_TCHAR), BUFSIZ, read_fp);
        }
        ::pclose(read_fp);
    }

    return result;
}

int SimMonitor::addMonitorSet(SimHandler *handler)
{
    if(handler)
    {
        MonitorSet *ms=0;
        ACE_NEW_RETURN(ms, MonitorSet(handler->getTid(),handler), -1);
        _monitor_sets.insert( ms );
    }
    return 0;
}

int SimMonitor::addMonitorSet(MonitorSet *ms)
{
    _monitor_sets.insert( ms );
    return 0;
}

int SimMonitor::deleteMonitorSet(MonitorSet *ms)
{
    if(ms)
    {
        _monitor_sets.remove(ms);
    }
    return 0;
}

int SimMonitor::deleteAllMonitorSets()
{
    MY_DEBUG("delete all monitors\n");

    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
//          MY_DEBUG ("try to delete monitor set, key=%s\n",(*iter)->getKey());
            MonitorSet* ms = (*iter);
            if(ms)
            {
//              MY_DEBUG ("delete monitor set, key=%s\n",(*iter)->getKey());
                delete ms;
                ms = 0;
                *iter = 0;
            }
        }
    }
    //need to unbind all here?

    return 0;
}

size_t SimMonitor::getNumOfNEs()
{
    size_t s = SIM_MANAGER->getSimNeMap().current_size();
    return s;
}

ACE_UINT64 SimMonitor::getTotalRecvDataSize()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getRecvDataSize().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getTotalSentDataSize()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentDataSize().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getTotalRecvMsgCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getRecvMsgCnt().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getTotalSentRplCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentRplCnt().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getTotalFailedMsgCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getFailedMsgCnt().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getTotalSentEvtCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentEvtCnt().getTotal();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentRecvDataSize()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getRecvDataSize().getCurrent();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentSentDataSize()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentDataSize().getCurrent();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentRecvMsgCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getRecvMsgCnt().getCurrent();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentSentRplCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentRplCnt().getCurrent();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentFailedMsgCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getFailedMsgCnt().getCurrent();
        }
    }
    return s;
}

ACE_UINT64 SimMonitor::getCurrentSentEvtCnt()
{
    ACE_UINT64 s = 0;
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            s += (*iter)->getSentEvtCnt().getCurrent();
        }
    }
    return s;
}

int SimMonitor::resetMonitorSets()
{
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            (*iter)->reset();
        }
    }    
    return 0;
}

int SimMonitor::updateMonitorSets()
{
    if(!_monitor_sets.is_empty())
    {
        for( SimMonitorSets::iterator
        iter = _monitor_sets.begin ();
        iter!= _monitor_sets.end ();
        iter++)
        {
            (*iter)->update();
        }
    }    
    return 0;
}
