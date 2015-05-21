#include "agent_action.h"
#include "log_msg.h"
#include "sim_monitor.h"
#include "sim_scheduler.h"
#include "common_utility.h"

AgentAction::AgentAction()
{
    MY_DEBUG("AgentAction::AgentAction()\n");
    init();
}

AgentAction::~AgentAction()
{
    MY_DEBUG("AgentAction::~AgentAction()\n");
}

int AgentAction::action(const AgentRequest& request, AgentResponse& response)
{
    ACE_CString action = request._request.action;
    if(_action_map.find(action)!=_action_map.end())
    {
        ActionFunc func = _action_map[action];
        return (this->*func)(request, response);
    }
    else
    {
        MY_ERROR("Cannot get the Action of %s\n",action.c_str());
        ActionFunc func = _action_map["noAction"];
        return (this->*func)(request, response);
    }
    return -1;
}

int AgentAction::init()
{
    ADD_AGENT_ACTION("noAction",noAction);
    ADD_AGENT_ACTION("shutdown",shutdown);
    ADD_AGENT_ACTION("GetSimulatorType",getSimType);
    ADD_AGENT_ACTION("GetNumberOfNEs",getNumOfNEs);
    ADD_AGENT_ACTION("GetHandledCommandCount",getHandledCmdCnt);
    ADD_AGENT_ACTION("GetOsStatus",getOsStatus);
    ADD_AGENT_ACTION("GetProcStatus",getProcStatus);
    ADD_AGENT_ACTION("GetSentEventCount",getSentEventCnt);
    ADD_AGENT_ACTION("SendAlarm",sendAlarm);

    return 0;
}

int AgentAction::noAction(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "ERR";
    response._response.except = "Cannot find the action";

    return -1;
}

int AgentAction::shutdown(const AgentRequest& request, AgentResponse& response )
{
    ACE_Reactor::instance()->end_reactor_event_loop ();
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";
    return 0;
}

int AgentAction::getSimType(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("type","7100"));

    return 0;
}

int AgentAction::getNumOfNEs(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";

    size_t num = SIM_MONITOR->getNumOfNEs();
    ACE_CString sNum = CommonUtility::str_size_t(num);
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("count",sNum));
    
    return 0;
}

int AgentAction::getHandledCmdCnt(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";
    
    SIM_MONITOR->updateMonitorSets();
    ACE_UINT64 recv_msg_cnt = SIM_MONITOR->getCurrentRecvMsgCnt();
    ACE_UINT64 failed_msg_cnt = SIM_MONITOR->getCurrentFailedMsgCnt();

    ACE_CString handledCount = CommonUtility::str_uint64(recv_msg_cnt);
    ACE_CString failedCount = CommonUtility::str_uint64(failed_msg_cnt);

    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("count",handledCount));
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("failedCount",failedCount));

    return 0;
}

int AgentAction::getOsStatus(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";

    double cpu = SIM_MONITOR->getOsCpuUsage();
    ACE_CString sCpu = CommonUtility::str_double(cpu, "%.2f") + "%";
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("CPU Usage",sCpu));

    double mem = SIM_MONITOR->getOsMemUsage();    
    ACE_CString sMem = CommonUtility::str_double(mem, "%.2f") + "%";
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("Memory Usage",sMem));
    
    return 0;
}

int AgentAction::getProcStatus(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";

    int nt = SIM_MONITOR->getProcNumThreads();
    ACE_CString sNt = CommonUtility::str_int(nt);
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("Number Of Threads",sNt));

    ACE_CString pcpu;
    ACE_CString pmem;
    ACE_CString vsz;
    ACE_CString rss;
    SIM_MONITOR->getProcUsage( pcpu, pmem, vsz, rss );

    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("CPU",pcpu));
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("Memory",pmem));
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("VSZ",vsz));
    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("RSS",rss));
    
    return 0;
}

int AgentAction::getSentEventCnt(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;
    response._response.result = "OK";
    
    SIM_MONITOR->updateMonitorSets();
    ACE_UINT64 sent_evt_cnt = SIM_MONITOR->getCurrentSentEvtCnt();

    ACE_CString sNum = CommonUtility::str_uint64(sent_evt_cnt);

    response._response.output.insert(std::pair<ACE_CString, ACE_CString>("count",sNum));

    return 0;
}

int AgentAction::sendAlarm(const AgentRequest& request, AgentResponse& response )
{
    response._version = request._version;
    response._response.req_id = request._request.id;
    response._response.action = request._request.action;

    ACE_CString swi;
    ACE_CString eventId;
    ACE_CString neTid;
 
    if( request._request.input.find("switch") != request._request.input.end())
    {
        swi = request._request.input.find("switch")->second;
    }
    if( request._request.input.find("alarmID") != request._request.input.end())
    {
        eventId = request._request.input.find("alarmID")->second;
    }
    if( request._request.input.find("NEID") != request._request.input.end())
    {
        neTid = request._request.input.find("NEID")->second;
    }

    if(swi.empty()||eventId.empty()||neTid.empty())
    {
        response._response.result = "ERR";
        response._response.except = "Missing parameter";
        return 0;
    }

    if(swi=="ON")
    {
        ACE_CString cmd = "start schedule -t";
        cmd += neTid + " -e" + eventId;
        ACE_ARGV args(cmd.c_str());
        int index=0;
        ACE_CString result;
        if( SCHEDULE_MANAGER->startSchedule(args, index, result) == -1 )
        {
            response._response.result = "ERR";
            response._response.except = result;
            return 0;
        }
    }
    else if(swi=="OFF")
    {
        ACE_CString cmd = "schedule -t";
        cmd += neTid + " -e" + eventId;
        ACE_ARGV args(cmd.c_str());
        ACE_CString result;
        ACE_CString key;
        int index;
        if(SCHEDULE_MANAGER->genKeyFromArgs(args, key) == -1 )
        {
            result = "schedule arguments parse error";
            response._response.result = "ERR";
            response._response.except = result;
            return 0;
        }
        if( (index=SCHEDULE_MANAGER->getIndexFromKey(key)) == -1 )
        {
            result = "schedule not exists";
            response._response.result = "ERR";
            response._response.except = result;
            return 0;
        }
        cmd = "stop schedule ";
        cmd += CommonUtility::str_int(index);
        ACE_ARGV stop_args(cmd.c_str());
        if( SCHEDULE_MANAGER->stopSchedule(stop_args, result) == -1 )
        {
            response._response.result = "ERR";
            response._response.except = result;
            return 0;
        }
    }
    else
    {
        response._response.result = "ERR";
        response._response.except = "Invalid parameter switch";
        return 0;
    }

    response._response.result = "OK";

    return 0;
}

