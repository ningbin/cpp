
#ifndef __SIM_MAP_H_
#define __SIM_MAP_H_

#include "ace/SString.h"
#include "ace/Hash_Map_Manager.h"
#include "ace/Map_Manager.h"
#include "ace/Synch.h" 
#include "ace/Functor.h"
#include "ace/Singleton.h"

typedef ACE_Map_Manager<ACE_CString, ACE_CString, ACE_Null_Mutex>
        Sim_Map;

class SimMap
{
    public:
        SimMap(const ACE_CString& filename);
        virtual ~SimMap();

        int init( void );
        int fini( void );
        int suspend( void );
        int resume( void );
        int getValue(const ACE_CString& key, ACE_CString& value);

        ACE_CString& getFile() {return _file; };

        int getCnt(){return _cnt; };
        int increaseCnt();
        int decreaseCnt();

        int tryDelete();

    private:
        SimMap();
        ACE_CString BeautifyText(const ACE_CString&);

        Sim_Map _map;
        ACE_CString _file;
        int _cnt;
        ACE_Thread_Mutex _lock;
};

#endif /* __SIM_MAP_H_ */
