
#ifndef __SIM_SCHEDULER_H_
#define __SIM_SCHEDULER_H_

#include "ace/Map_Manager.h"
#include "ace/Functor.h"
#include "ace/Synch.h"
#include "ace/Task.h"
#include "ace/ARGV.h"
#include <list>

class SimScheduler : public ACE_Task_Base
{
    public:
    	SimScheduler(const ACE_CString& key, const ACE_CString& args, int index );                     
    	virtual ~SimScheduler();
        
        virtual int handle_timeout (const ACE_Time_Value &current_time, const void *arg);
        virtual int handle_close (ACE_HANDLE h, ACE_Reactor_Mask mask);
        
        virtual int svc(void);

        virtual int start(ACE_CString& result);
        virtual int stop(ACE_CString& result);

        virtual int pause(ACE_CString& result);
        virtual int proceed(ACE_CString& result);
        
        virtual int action(void);

        bool running(){return _running;}
        ACE_CString& getKey(){return _key; };
        ACE_CString& getArgs(){return _args; };

        int getInterval(){return _interval; };
        ACE_CString getTid(){return _tid; };
        ACE_CString getEventId(){return _event_id; };
        ACE_CString getConf(){return _conf; };
        ACE_UINT64 getSentEvtCnt(){return _sent_evt_cnt; };

    protected:
        SimScheduler(); 
        int parseArgs(void);
        int loadEvents(void);   
        ACE_CString trimText(const ACE_CString& data); 
        
        ACE_CString _key;
        ACE_CString _args;
        int _index;

        int _interval;
        ACE_CString _tid;
        ACE_CString _event_id;
        ACE_CString _conf;
        std::list<ACE_CString> _event_list;

        ACE_UINT64 _sent_evt_cnt;

        bool _running;
        bool _stopped;
        ACE_Thread_Semaphore _notempty;
};

typedef ACE_Map_Manager<int, SimScheduler*, ACE_Null_Mutex>
        Scheduler_Map;

typedef ACE_Map_Manager<ACE_CString, int, ACE_Null_Mutex>
        Index_Map;

class SimSchedulerManager
{
    public:
    	SimSchedulerManager();                            
    	virtual ~SimSchedulerManager();

        int fini();
        
        SimScheduler* getSchedule(int index);
        int startSchedule( ACE_ARGV& args, int& index, ACE_CString& result );
        int stopSchedule( ACE_ARGV& args, ACE_CString& result  );
        int stopAllSchedules(ACE_CString& result);
        
        int getIndex();
        int getIndexFromKey(const ACE_CString& key);
        Scheduler_Map& getScheduleMap(){return _map;};
        Index_Map& getIndexMap(){return _index_map;};

        int genKeyFromArgs(ACE_ARGV& args, ACE_CString& key);
        
    private:
        Scheduler_Map _map;
        Index_Map _index_map;

        int _index;
        ACE_Thread_Mutex _lock;
};

typedef ACE_Singleton<SimSchedulerManager, ACE_Null_Mutex>
        SimSchedulerManagerSingleton;
#define SCHEDULE_MANAGER SimSchedulerManagerSingleton::instance()

#endif /* __SIM_SCHEDULER_H_ */
