#include "ace/Reactor.h"
#include "ace/Get_Opt.h"
#include "agent_ui.h"
#include "log_msg.h"
#include "sim_service.h"
#include "sim_monitor.h"
#include "sim_scheduler.h"
#include "sim_manager.h"
#include "common_utility.h"

UIHandler::UIHandler()
{
    MY_DEBUG("UIHandler::UIHandler()\n");
}

UIHandler::~UIHandler()
{
    MY_DEBUG("UIHandler::~UIHandler()\n");
}

int UIHandler::handle_input(ACE_HANDLE handle )
{
//  MY_DEBUG("UIHandler::handle_input\n");
    ACE_TCHAR buf[1024];
    ACE_OS::memset(buf,0,sizeof buf);
    ACE_OS::read(handle, buf, sizeof buf);
    for(size_t i=0; i<sizeof buf; i++)
    {
        if(buf[i]=='\n')
        {
            buf[i]='\0';
            break;
        }
    }

    ACE_ARGV args(buf);
    if(args.argc()==0)
    {
        AGENT_UI->action("NULL", args);
    }
    else
    {
        ACE_CString cmd = args.argv()[0];
        if(args.argc()>1)
        {
            cmd += " ";
            cmd += args.argv()[1];
        }

        AGENT_UI->action(cmd, args);
    }

    return 0;
}

int UIHandler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask m)
{
    MY_DEBUG("UIHandler::handle_close\n");
    ACE_Reactor::instance()->remove_handler(this, 
                                ACE_Event_Handler::READ_MASK |
                                ACE_Event_Handler::DONT_CALL);

    return super::handle_close(h,m);
}

ActionDescription::ActionDescription(const ACE_TCHAR* name, 
                                     const ACE_TCHAR* desc,
                                     bool help)
                  :_name(name),_desc(desc),_help(help)
{}

AgentUI::AgentUI() : _running(false)
{
    MY_DEBUG("AgentUI::AgentUI()\n");
    init();
}

AgentUI::~AgentUI()
{
    MY_DEBUG("AgentUI::~AgentUI()\n");
    if(_running)
    {
        this->stop();
    }
}

int AgentUI::action(const ACE_CString& key,  ACE_ARGV& args)
{
    if(key!="/")
    {
        _lastCmd = args.buf();
    }
    if(_action_map.find(key)!=_action_map.end())
    {
        UIFunc func = _action_map[key];
        return (this->*func)(args);
    }
    else
    {
        MY_DEBUG("No Action = %s\n",key.c_str());
        UIFunc func = _action_map["defaultAction"];
        if(func)
        {
            return (this->*func)(args);
        }
    }
    return -1;
}

int AgentUI::init()
{
    ADD_UI_ACTION("NULL",nullAction,"null action",false);
    ADD_UI_ACTION("/",lastAction,"redo last action",true);
    ADD_UI_ACTION("defaultAction",defaultAction,"default action",false);
    ADD_UI_ACTION("help", helpAction,"list all commands",true);
    ADD_UI_ACTION("quit",quitAction,"quit UI",true);
    ADD_UI_ACTION("exit",quitAction,"exit UI",true);
    ADD_UI_ACTION("show os",getOsStatus,"show OS status",true);
    ADD_UI_ACTION("show process",getProcStatus,"show process status",true);
    ADD_UI_ACTION("start schedule",startSchedule,"start schedule",true);
    ADD_UI_ACTION("stop schedule",stopSchedule,"stop schedule",true);
    ADD_UI_ACTION("show schedule",showSchedule,"show schedule status",true);
    ADD_UI_ACTION("show ne",showNE,"show NE status",true);
    ADD_UI_ACTION("start ne", startNE,"start NE",true);
    ADD_UI_ACTION("stop ne", stopNE,"stop NE",true);
    ADD_UI_ACTION("show connection", showConnection,"show connection status",true);
    ADD_UI_ACTION("show total", showTotal,"show total statistics",true);
    ADD_UI_ACTION("show status", showStatus,"show current statistics",true);
    ADD_UI_ACTION("reset status", resetStatus,"reset current statistics",true);
    return 0;
}

int AgentUI::start()
{
    MY_DEBUG("AgentUI::start\n");
    if(!_running)
    {
        ACE_CString reply(UIPrompt);
        ACE_OS::write_n (ACE_STDIN, reply.c_str(), reply.length());

        ACE_NEW_RETURN(_handler, UIHandler, -1);
        ACE_Reactor::instance()->register_handler(ACE_STDIN,
                                 _handler,
                                 ACE_Event_Handler::READ_MASK);
    }
    _running = true;
    return 0;
}

int AgentUI::stop()
{
    MY_DEBUG("AgentUI::stop\n");
    if(_running)
    {
        delete _handler;
        _handler = 0;
    }
    _running = false;
    return 0;
}

const ACE_TCHAR * AgentUI::UIPrompt = "[SIM]$ ";


int AgentUI::reply(const ACE_CString& msg)
{
    return ACE_OS::write_n( ACE_STDIN, msg.c_str(),msg.length());
}

int AgentUI::nullAction( ACE_ARGV& args )
{
    ACE_CString reply(UIPrompt);
    this->reply(reply);

    return 0;
}

int AgentUI::lastAction( ACE_ARGV& args )
{
    ACE_ARGV lastArgs(_lastCmd.c_str());
    if(_lastCmd.empty())
    {
        return this->action("NULL", lastArgs);
    }
    else
    {
        ACE_CString cmd = lastArgs.argv()[0];
        if(lastArgs.argc()>1)
        {
            cmd += " ";
            cmd += lastArgs.argv()[1];
        }
        return this->action(cmd, lastArgs);
    }
}

int AgentUI::defaultAction( ACE_ARGV& args )
{
    ACE_CString reply("\t");
    reply += "Invalid Command : ";
    reply += args.buf();
    reply += "\n";
    reply += UIPrompt;
    this->reply(reply);

    return -1;
}

int AgentUI::helpAction( ACE_ARGV& args )
{
    ACE_CString reply;
    for(std::list<ActionDescription>::iterator
        it = _desc_list.begin();
        it!= _desc_list.end();
        ++it)
    {
        if(it->_help)
        {
            reply += "\t" + it->_name + " : ";
            reply += it->_desc + "\n";
        }
    }
    reply += UIPrompt;
    
    this->reply(reply);
    return 0;
}

int AgentUI::quitAction( ACE_ARGV& args )
{
    MY_DEBUG("AgentUI::quitAction\n");
    ACE_Reactor::instance()->end_reactor_event_loop ();

    return 0;
}

int AgentUI::getOsStatus( ACE_ARGV& args)
{
    ACE_CString reply("\tOS Platform = ");
    ACE_CString osUname;
    SIM_MONITOR->getOsUname(osUname);
    reply += osUname + "\n";

    reply += "\tCPU Usage = ";
    double cpu = SIM_MONITOR->getOsCpuUsage();
    reply += CommonUtility::str_double(cpu,"%.2f") + "%\n";

    double mem = SIM_MONITOR->getOsMemUsage();
    reply += "\tMemory Usage = ";
    reply += CommonUtility::str_double(mem,"%.2f") + "%\n";
    reply += UIPrompt;
    
    this->reply(reply);

    return 0;
}

int AgentUI::getProcStatus( ACE_ARGV& args )
{
    pid_t pid = ACE_OS::getpid(); 
    ACE_CString reply("\tPID = "); 
    reply += CommonUtility::str_int(pid) + "\n";

    reply += "\tTheads Total = ";
    int ts = SIM_MONITOR->getProcNumThreads();
    reply += CommonUtility::str_int(ts);
    reply += "\n";
    
    ACE_CString pcpu;
    ACE_CString pmem;
    ACE_CString vsz;
    ACE_CString rss;
    SIM_MONITOR->getProcUsage( pcpu, pmem, vsz, rss );

    reply += "\tCPU% = ";
    reply += pcpu + "\n";
    reply += "\tMEM% = ";
    reply += pmem + "\n";
    reply += "\tVSZ  = ";
    reply += vsz + " KB\n";
    reply += "\tRSS  = ";
    reply += rss + " KB\n";
    reply += UIPrompt;

    this->reply(reply);

    return 0;
}

int AgentUI::startSchedule( ACE_ARGV& args )
{
    ACE_CString reply("\t");
    if(args.argc()<3)
    {
        reply += "Invalid Command, missing parameters\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    int index = 0;
    ACE_CString result;
    if( SCHEDULE_MANAGER->startSchedule(args, index, result) == -1)
    {
        reply += "Command failed!\n";
        reply += "\t" + result + "\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    else
    {
        reply += "schedule ";
        reply += CommonUtility::str_int(index);
        reply += " started!\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }

    return 0;
}

int AgentUI::stopSchedule( ACE_ARGV& args )
{
    ACE_CString reply("\t");
    if(args.argc()<3)
    {
        reply += "Invalid Command, missing parameters\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    
    ACE_CString result;
    if( SCHEDULE_MANAGER->stopSchedule(args, result) == -1)
    {
        reply += "Command failed!\n";
        reply += "\t" + result + "\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    else
    {
        reply += "schedule ";
        reply += args.argv()[2];
        reply += " stopped!\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    return 0;
}

int AgentUI::showSchedule( ACE_ARGV& args )
{
    ACE_CString reply("\tSchedules total = ");
    Scheduler_Map& schedule_map = SCHEDULE_MANAGER->getScheduleMap();
    int map_size = schedule_map.current_size();
    reply += CommonUtility::str_int(map_size);
    reply += "\n";

    for( Scheduler_Map::iterator it =
         schedule_map.begin();
         it!=schedule_map.end();
         it++)
    {
        reply += "\tschedule ";
        reply += CommonUtility::str_int((*it).ext_id_);
        reply += " : " + ((*it).int_id_)->getKey() + "\n";
        reply += "\t\ttid        = "+ ((*it).int_id_)->getTid() + "\n";
        reply += "\t\tevent_id   = "+ ((*it).int_id_)->getEventId() + "\n";
        reply += "\t\tinterval   = "+ CommonUtility::str_int(((*it).int_id_)->getInterval()) + " ms\n";
        reply += "\t\tfile       = "+ ((*it).int_id_)->getConf() + "\n";
        reply += "\t\tsent_event = "+ CommonUtility::str_uint64(((*it).int_id_)->getSentEvtCnt()) + "\n";
    }
    reply += UIPrompt;
    this->reply(reply);

    return 0;
}

int AgentUI::showNE( ACE_ARGV& args )
{
    ACE_CString reply("\tNE total = ");
    SimNE_Map& ne_map = SIM_MANAGER->getSimNeMap();
    int map_size = ne_map.current_size();
    reply += CommonUtility::str_int(map_size);
    reply += "\n";

    for( SimNE_Map::iterator it =
         ne_map.begin();
         it!=ne_map.end();
         it++)
    {
        reply += "\t" + (*it).ext_id_ + " : port = " + CommonUtility::str_int( (*(*it).int_id_)._port );
        reply += "\n\t\ttype = " + (*(*it).int_id_)._type;        
        reply += ", gne = " + (*(*it).int_id_)._gne;        
        reply += ", config = " + (*(*it).int_id_)._conf;
        reply += "\n";
    }
    if(map_size>0)
    {
        reply += "\tNE total = ";
        reply += CommonUtility::str_int(map_size);
        reply += "\n";
    }
    reply += UIPrompt;
    this->reply(reply);

    return 0;
}

int AgentUI::startNE( ACE_ARGV& args )
{
    ACE_CString reply("\t");
    if(args.argc()<3)
    {
        reply += "Invalid Command, missing parameters\n";
        reply += UIPrompt;
        this->reply(reply);
        return -1;
    }
    ACE_Get_Opt get_opt (args.argc(), args.argv(), ACE_TEXT ("c:"), 0);
    get_opt.long_option (ACE_TEXT ("config"),'c');
    ACE_TCHAR conf[MAXPATHLEN];
    bool has_conf = false;
    for (int c; (c = get_opt ()) != -1; )
    switch (c) {
        case 'c':
          ACE_OS::strsncpy
            (conf, get_opt.opt_arg (), MAXPATHLEN);
          has_conf = true;
         	break;
    }
    if(!has_conf)
    {
        ACE_OS::strsncpy(conf, "sim.xml", MAXPATHLEN);
    }
    ACE_CString tid = args.argv()[2];
    if(tid == "all")
    {

        if( SIM_MANAGER->startAllNE(conf)==-1 )
        {
            reply += "start NE all failed!\n";
            reply += UIPrompt;
            this->reply(reply);
            return -1;
        }
        else
        {
            reply += "start NE all successfully!\n";
            reply += UIPrompt;
            this->reply(reply);
        }
    }
    else
    {
        ACE_CString reply("\t //TODO\n");
        reply += UIPrompt;
        this->reply(reply);
    
        return 0;
    }
    return 0;
}

int AgentUI::stopNE( ACE_ARGV& args )
{
    ACE_CString reply("\t //TODO\n");
    reply += UIPrompt;
    this->reply(reply);

    return 0;
}

int AgentUI::showConnection( ACE_ARGV& args )
{
    ACE_CString head("\tConnection total = ");
    ACE_CString body("\n");
    SimHandler_Map& handler_map = SIM_MANAGER->getSimHandlerMap();
    int handler_sum = 0;

    for( SimHandler_Map::iterator it =
         handler_map.begin();
         it!=handler_map.end();
         it++)
    {
        SimHandlerSet* handler_set = (*it).int_id_;
        ACE_CString tid = (*it).ext_id_;
        body += "\t" + tid + " : ";
        if(handler_set)
        {
            int num = handler_set->size();         
            if(num>0)
            {
                bool firstConnect = true;
                for(SimHandlerSet::iterator
                    iter = handler_set->begin();
                    iter!= handler_set->end();
                    ++iter)
                {
                    SimHandler* h = *iter;
                    if(h)
                    {
                        if(!firstConnect)
                        {
                            body += "\n\t\t";
                        }
                        body += "[" + h->getPeerIp() + ":" 
                             + CommonUtility::str_int(h->getPeerPort() )
                             + "]";
                        if(firstConnect)
                        {
                            firstConnect = false;
                        }
                    }
                }
                handler_sum += num;
            }
        }
        body += "\n";
    }
    head += CommonUtility::str_int(handler_sum);

    ACE_CString reply = head + body + UIPrompt;
    this->reply(reply);

    return 0;
}

int AgentUI::showTotal( ACE_ARGV& args )
{
    SIM_MONITOR->updateMonitorSets();
    ACE_UINT64 recv_msg_cnt = SIM_MONITOR->getTotalRecvMsgCnt();
    ACE_UINT64 sent_rpl_cnt = SIM_MONITOR->getTotalSentRplCnt();
    ACE_UINT64 failed_msg_cnt = SIM_MONITOR->getTotalFailedMsgCnt();
    ACE_UINT64 sent_evt_cnt = SIM_MONITOR->getTotalSentEvtCnt();

    ACE_UINT64 recv_data_size = SIM_MONITOR->getTotalRecvDataSize();
    ACE_UINT64 sent_data_size = SIM_MONITOR->getTotalSentDataSize();

    ACE_CString reply("\tTotal Received Message Count = ");
    reply += CommonUtility::str_uint64(recv_msg_cnt) + "\n";
    reply += "\tTotal Reply Message Count = ";
    reply += CommonUtility::str_uint64(sent_rpl_cnt) + "\n";
    reply += "\tTotal Failed Message Count = ";
    reply += CommonUtility::str_uint64(failed_msg_cnt) + "\n";
    reply += "\tTotal Sent Event Count = ";
    reply += CommonUtility::str_uint64(sent_evt_cnt) + "\n";

    reply += "\tTotal Received Data Size = ";
    reply += CommonUtility::str_uint64(recv_data_size) + "\n";
    reply += "\tTotal Sent Data Size = ";
    reply += CommonUtility::str_uint64(sent_data_size) + "\n";

    reply += UIPrompt;
    this->reply(reply);
    return 0;
}

int AgentUI::showStatus( ACE_ARGV& args )
{
    SIM_MONITOR->updateMonitorSets();
    ACE_UINT64 recv_msg_cnt = SIM_MONITOR->getCurrentRecvMsgCnt();
    ACE_UINT64 sent_rpl_cnt = SIM_MONITOR->getCurrentSentRplCnt();
    ACE_UINT64 failed_msg_cnt = SIM_MONITOR->getCurrentFailedMsgCnt();
    ACE_UINT64 sent_evt_cnt = SIM_MONITOR->getCurrentSentEvtCnt();

    ACE_UINT64 recv_data_size = SIM_MONITOR->getCurrentRecvDataSize();
    ACE_UINT64 sent_data_size = SIM_MONITOR->getCurrentSentDataSize();

    ACE_CString reply("\tReceived Message Count = ");
    reply += CommonUtility::str_uint64(recv_msg_cnt) + "\n";
    reply += "\tReply Message Count = ";
    reply += CommonUtility::str_uint64(sent_rpl_cnt) + "\n";
    reply += "\tFailed Message Count = ";
    reply += CommonUtility::str_uint64(failed_msg_cnt) + "\n";
    reply += "\tSent Event Count = ";
    reply += CommonUtility::str_uint64(sent_evt_cnt) + "\n";

    reply += "\tReceived Data Size = ";
    reply += CommonUtility::str_uint64(recv_data_size) + "\n";
    reply += "\tSent Data Size = ";
    reply += CommonUtility::str_uint64(sent_data_size) + "\n";

    reply += UIPrompt;
    this->reply(reply);

    return 0;
}

int AgentUI::resetStatus( ACE_ARGV& args )
{
    SIM_MONITOR->resetMonitorSets();
    SIM_MONITOR->updateMonitorSets();
    ACE_UINT64 recv_msg_cnt = SIM_MONITOR->getCurrentRecvMsgCnt();
    ACE_UINT64 sent_rpl_cnt = SIM_MONITOR->getCurrentSentRplCnt();
    ACE_UINT64 failed_msg_cnt = SIM_MONITOR->getCurrentFailedMsgCnt();
    ACE_UINT64 sent_evt_cnt = SIM_MONITOR->getCurrentSentEvtCnt();

    ACE_UINT64 recv_data_size = SIM_MONITOR->getCurrentRecvDataSize();
    ACE_UINT64 sent_data_size = SIM_MONITOR->getCurrentSentDataSize();

    ACE_CString reply("\tReceived Message Count = ");
    reply += CommonUtility::str_uint64(recv_msg_cnt) + "\n";
    reply += "\tReply Message Count = ";
    reply += CommonUtility::str_uint64(sent_rpl_cnt) + "\n";
    reply += "\tFailed Message Count = ";
    reply += CommonUtility::str_uint64(failed_msg_cnt) + "\n";
    reply += "\tSent Event Count = ";
    reply += CommonUtility::str_uint64(sent_evt_cnt) + "\n";

    reply += "\tReceived Data Size = ";
    reply += CommonUtility::str_uint64(recv_data_size) + "\n";
    reply += "\tSent Data Size = ";
    reply += CommonUtility::str_uint64(sent_data_size) + "\n";

    reply += UIPrompt;
    this->reply(reply);

    return 0;
}
