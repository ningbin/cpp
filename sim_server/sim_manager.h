
#ifndef __SIM_MANAGER_H_
#define __SIM_MANAGER_H_

#include "ace/SString.h"
#include "ace/Hash_Map_Manager.h"
#include "ace/Map_Manager.h"
#include "ace/Synch.h" 
#include "ace/Functor.h"
#include "ace/Singleton.h"
#include "ace/Containers.h"
#include "sim_map.h"
#include <list>

class SimHandler;
class SimAcceptor;

typedef ACE_Unbounded_Set<SimHandler*> SimHandlerSet;

typedef ACE_Map_Manager<ACE_CString, SimHandlerSet*, ACE_Null_Mutex>
        SimHandler_Map;

typedef ACE_Map_Manager<ACE_CString, SimAcceptor*, ACE_Null_Mutex>
        SimAcceptor_Map;

typedef ACE_Map_Manager<ACE_CString, SimMap*, ACE_Null_Mutex>
        SimMap_Map;

class SimNE;

typedef ACE_Map_Manager<ACE_CString, SimNE*, ACE_Null_Mutex>
        SimNE_Map;

class SimManager
{
    public:
        SimManager();
        virtual ~SimManager();

        int fini( void );

        SimHandlerSet* getSimHandlerSet(const ACE_CString& key);
        int addSimHandler(const ACE_CString& key, SimHandler *handler);
        int removeSimHandler(const ACE_CString& key, SimHandler *handler);        
        int deleteAllSimHandlerSet(void);
        int findSimHandler(const ACE_CString& key, SimHandler* handler);

        SimAcceptor* getSimAccetpor(const ACE_CString& key); 
        int addSimAcceptor(const ACE_CString& key, SimAcceptor *acceptor);
        int removeSimAcceptor(const ACE_CString& key, SimAcceptor *acceptor=0 );         

        SimMap* getSimMap(const ACE_CString& file);
        int createSimMap(const ACE_CString& file );
        int deleteSimMap(const ACE_CString& file );
        int deleteAllSimMaps(void);

        SimNE* getSimNE(const ACE_CString& tid);
        int createSimNE(const ACE_CString& tid, const ACE_CString& type, 
                        const ACE_CString& gne, const ACE_CString& conf,
                        int port, const ACE_CString& tidIpMap = "");
        int deleteSimNE(const ACE_CString& tid);
        int deleteAllSimNEs(void);
        int startAllNE(const ACE_TCHAR* conf);
        int startNE( const ACE_CString& tid, const ACE_CString& type, 
                   const ACE_CString& gne, const ACE_CString& conf, 
                   int port, const ACE_CString& tidIpMap = "");

        int getTidAndConfList(const ACE_CString& tid, 
                              std::list<ACE_CString>& tidList, std::list<ACE_CString>& confList);
        
        SimHandler_Map& getSimHandlerMap(){return _handler_map;};
        SimAcceptor_Map& getSimAcceptorMap(){return _acceptor_map;};
        SimNE_Map& getSimNeMap(){return _ne_map;};
        SimMap_Map& getSimMapMap(){return _map_map;};

    private:
        SimHandler_Map _handler_map;
        SimAcceptor_Map _acceptor_map;
        SimNE_Map _ne_map;
        SimMap_Map _map_map;
};

typedef ACE_Singleton<SimManager, ACE_Null_Mutex>
        SimManagerSingleton;
#define SIM_MANAGER SimManagerSingleton::instance()

class SimNE
{
    public:
        SimNE(const ACE_CString& tid, const ACE_CString& type, 
              const ACE_CString& gne, const ACE_CString& conf,
              int port, SimManager* manager = SIM_MANAGER, 
              const ACE_CString& tidIpMap = "" );
        ~SimNE();

        int _port;
        ACE_CString _tid;
        ACE_CString _conf;
        ACE_CString _gne;
        ACE_CString _type;

        SimMap* _sim_map;

        SimMap* getSimMap();
        SimManager* getManager(){return _manager;};
        ACE_CString& getTidIpMap(){return _tidIpMap; };

    private:
        SimNE();
        SimManager* _manager;
        ACE_CString _tidIpMap;

};


#endif /* __SIM_MANAGER_H_ */
