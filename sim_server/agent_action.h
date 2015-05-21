
#ifndef __AGENT_ACTION_H_
#define __AGENT_ACTION_H_

#include "ace/Synch.h" 
#include "ace/Singleton.h"
#include <map>

#include "agent_request.h"
#include "agent_response.h"

class AgentAction;

typedef int (AgentAction::*ActionFunc)(const AgentRequest& request, AgentResponse& response );

class AgentAction
{
    public:
        AgentAction();
        ~AgentAction();

        int action(const AgentRequest& request, AgentResponse& response );

    private:
        int init(void);
        std::map<ACE_CString, ActionFunc> _action_map;

        int noAction(const AgentRequest& request, AgentResponse& response );
        int shutdown(const AgentRequest& request, AgentResponse& response );
        int getSimType(const AgentRequest& request, AgentResponse& response );
        int getNumOfNEs(const AgentRequest& request, AgentResponse& response );
        int getHandledCmdCnt(const AgentRequest& request, AgentResponse& response );
        int getOsStatus(const AgentRequest& request, AgentResponse& response );
        int getProcStatus(const AgentRequest& request, AgentResponse& response );
        int getSentEventCnt(const AgentRequest& request, AgentResponse& response );
        int sendAlarm(const AgentRequest& request, AgentResponse& response );
};

#define ADD_AGENT_ACTION(NAME, FUNC) \
    _action_map.insert(std::pair<ACE_CString, ActionFunc>(NAME,&AgentAction::FUNC))

typedef ACE_Singleton<AgentAction, ACE_Null_Mutex>
        AgentActionSingleton;
#define AGENT_ACTION AgentActionSingleton::instance()

#endif /* __AGENT_ACTION_H_ */

