
#ifndef __SIM_MONITOR_H_
#define __SIM_MONITOR_H_

#include "ace/SString.h"
#include "ace/Map_Manager.h"
#include "ace/Synch.h" 
#include "ace/Functor.h"
#include "ace/Singleton.h"
#include "ace/Containers.h"
#include "ace/Monitor_Control/Monitor_Control.h"
#include "sim_handler.h"

class MonitorPoint
{
    public:
        MonitorPoint();
        virtual ~MonitorPoint();

        void reset(){_base = _total;};
        void update(ACE_UINT64 value){
            _total = value;
            _current = _total-_base;
        };

        ACE_UINT64 getCurrent(){return _current;};
        ACE_UINT64 getTotal(){return _total;};
        ACE_UINT64 getBase(){return _base;};

    private:
        ACE_UINT64 _current;
        ACE_UINT64 _total;
        ACE_UINT64 _base;
};

class MonitorSet
{
    public:
        MonitorSet(const ACE_CString& key, SimHandler* handler);
        virtual ~MonitorSet();
        ACE_CString& getKey(){return _key;};

        void reset();
        void update();

        MonitorPoint& getRecvMsgCnt(){return _recvMsgCnt;};
        MonitorPoint& getSentRplCnt(){return _sentRplCnt;};
        MonitorPoint& getFailedMsgCnt(){return _failedMsgCnt;};
        MonitorPoint& getSentEvtCnt(){return _sentEvtCnt;};
        MonitorPoint& getRecvDataSize(){return _recvDataSize;};
        MonitorPoint& getSentDataSize(){return _sentDataSize;};

    private:
        MonitorPoint _recvMsgCnt;
        MonitorPoint _sentRplCnt;
        MonitorPoint _failedMsgCnt;
        MonitorPoint _sentEvtCnt;
        MonitorPoint _recvDataSize;
        MonitorPoint _sentDataSize;

        SimHandler* _handler;
        ACE_CString _key;
};

typedef ACE_Unbounded_Set<MonitorSet*> SimMonitorSets;

class SimMonitor
{
    public:
        SimMonitor();
        virtual ~SimMonitor();
        
        int init( void );
        int fini( void );

        int getOsUname(ACE_CString& uname);
        double getOsCpuUsage();
        double getOsMemUsage();
        int getProcNumThreads();
        int getProcUsage(ACE_CString& pcpu, ACE_CString& pmem,
                         ACE_CString& vsz, ACE_CString& rss);

        ACE_UINT64 getTotalRecvMsgCnt();
        ACE_UINT64 getTotalSentRplCnt();
        ACE_UINT64 getTotalFailedMsgCnt();
        ACE_UINT64 getTotalSentEvtCnt();
        ACE_UINT64 getTotalRecvDataSize();
        ACE_UINT64 getTotalSentDataSize();

        ACE_UINT64 getCurrentRecvMsgCnt();
        ACE_UINT64 getCurrentSentRplCnt();
        ACE_UINT64 getCurrentFailedMsgCnt();
        ACE_UINT64 getCurrentSentEvtCnt();
        ACE_UINT64 getCurrentRecvDataSize();
        ACE_UINT64 getCurrentSentDataSize();
        
        int resetMonitorSets();
        int updateMonitorSets();

        ACE_CString getExecOutput(const ACE_TCHAR* cmd);

        size_t getNumOfNEs();

        int addMonitorSet(SimHandler *handler);
        int addMonitorSet(MonitorSet *ms);
        int deleteMonitorSet(MonitorSet *ms);
        int deleteAllMonitorSets();
        
        Monitor_Base * getCpuLoadMonitor(){return _cpu_load_monitor;};
        Monitor_Base * getMemoryUsageMonitor(){return _memory_usage_monitor;};
        Monitor_Base * getNumThreadsMonitor(){return _num_threads_monitor;};
        SimMonitorSets& getSimMonitorSets(){return _monitor_sets;};

    private:
        Monitor_Base *_cpu_load_monitor;
        Monitor_Base *_memory_usage_monitor;
        Monitor_Base *_num_threads_monitor;
        SimMonitorSets _monitor_sets;
};

typedef ACE_Singleton<SimMonitor, ACE_Null_Mutex>
        SimMonitorSingleton;
#define SIM_MONITOR SimMonitorSingleton::instance()

#endif /* __SIM_MONITOR_H_ */
