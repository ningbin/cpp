
#ifndef __AGENT_UI_H_
#define __AGENT_UI_H_

#include "ace/SString.h" 
#include "ace/Synch.h" 
#include "ace/Singleton.h"
#include "ace/Event_Handler.h"
#include "ace/ARGV.h"
#include <map>
#include <list>

class AgentUI;
typedef int (AgentUI::*UIFunc)( ACE_ARGV& args );

class UIHandler : public ACE_Event_Handler
{
    typedef ACE_Event_Handler super;
    public:
        UIHandler();
        ~UIHandler();

        virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
        virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
        
    private:
    
};

struct ActionDescription
{
    ActionDescription(const ACE_TCHAR* name, 
                      const ACE_TCHAR* desc = "",
                      bool help = false );
    ACE_CString _name;
    ACE_CString _desc;
    bool _help;
};

class AgentUI
{
    public:
        AgentUI();
        virtual ~AgentUI();

        virtual int start();
        virtual int stop();
        virtual int action(const ACE_CString& key, ACE_ARGV& args );
        
        bool running(){return _running;};

    protected:
        int init(void);
        std::map<ACE_CString, UIFunc> _action_map;
        std::list<ActionDescription> _desc_list;
        int reply(const ACE_CString& msg);

        int nullAction( ACE_ARGV& args );
        int lastAction( ACE_ARGV& args );
        int defaultAction( ACE_ARGV& args );
        int helpAction( ACE_ARGV& args );
        int quitAction( ACE_ARGV& args );
        int getOsStatus( ACE_ARGV& args );
        int getProcStatus( ACE_ARGV& args );
        int startSchedule( ACE_ARGV& args );
        int stopSchedule( ACE_ARGV& args );
        int showSchedule( ACE_ARGV& args );
        int showNE( ACE_ARGV& args );
        int startNE( ACE_ARGV& args );
        int stopNE( ACE_ARGV& args );
        int showConnection( ACE_ARGV& args );
        int showTotal( ACE_ARGV& args );
        int showStatus( ACE_ARGV& args );
        int resetStatus( ACE_ARGV& args );
        
        UIHandler* _handler;
        bool _running;
        ACE_CString _lastCmd;

        static const ACE_TCHAR *UIPrompt;
};

#define ADD_UI_ACTION(NAME, FUNC, DESC, HELP) \
    _action_map.insert(std::pair<ACE_CString, UIFunc>(NAME,&AgentUI::FUNC)); \
    _desc_list.push_back( ActionDescription(NAME,DESC,HELP) )

typedef ACE_Singleton<AgentUI, ACE_Null_Mutex>
        AgentUISingleton;
#define AGENT_UI AgentUISingleton::instance()

#endif /* __AGENT_UI_H_ */

