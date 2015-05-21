

#include "ace/Service_Config.h"
#include "ace/Service_Repository.h"
#include "ace/Service_Types.h"
#include "sim_service.h"
#include "ace/Get_Opt.h"
#include "ace/INET_Addr.h"
#include "log_msg.h"
#include "sim_manager.h"
#include <list>

SimAcceptor::SimAcceptor(const ACE_CString& tid, int port):
            _tid(tid), _port(port)
{
	MY_DEBUG( "SimAcceptor::SimAcceptor()\n" );
}

SimAcceptor::~SimAcceptor()
{
	MY_DEBUG( "SimAcceptor::~SimAcceptor()\n" );
}

int SimAcceptor::handle_close (ACE_HANDLE, ACE_Reactor_Mask )
{
	MY_DEBUG( "SimAcceptor::handle_close()\n" );

    SIM_MANAGER->removeSimAcceptor(_tid,this);
    std::list<ACE_CString> tidList;
    std::list<ACE_CString> confList;
    SIM_MANAGER->getTidAndConfList(_tid, tidList, confList);
    
    for(std::list<ACE_CString>::iterator
        it = confList.begin();
        it!= confList.end();
        ++it)
    {
        SIM_MANAGER->deleteSimMap(*it);
    }
    for(std::list<ACE_CString>::iterator
        it = tidList.begin();
        it!= tidList.end();
        ++it)
    {
        SIM_MANAGER->deleteSimNE(*it);
    }

    delete this;
	return 0;
}

int SimService::init (int argc, ACE_TCHAR *argv[])
{
    MY_TRACE( ACE_TEXT ("SimService::init\n") );

    ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("c:s"), 0);
    get_opt.long_option (ACE_TEXT ("config"),
                       'c',
                       ACE_Get_Opt::ARG_REQUIRED);
    get_opt.long_option (ACE_TEXT ("start-all-ne"),
                       's');
    
    ACE_TCHAR conf[MAXPATHLEN];
    bool startAllNe = false;
    for (int c; (c = get_opt ()) != -1; )
    switch (c) {
        case 'c':
          ACE_OS::strsncpy
            (conf, get_opt.opt_arg (), MAXPATHLEN);
         	break;
        case 's':
            startAllNe = true;
         	break;
    }    
    if(startAllNe)
    {
        return SIM_MANAGER->startAllNE(conf);
    }
    return 0;
}

int SimService::fini ()
{
    MY_TRACE( ACE_TEXT ("SimService::fini\n") );
    return 0;
}

int SimService::info (ACE_TCHAR **, size_t) const
{
	return 0;
}

int SimService::suspend ()
{
    SimAcceptor_Map& acceptor_map = SIM_MANAGER->getSimAcceptorMap();
    for( SimAcceptor_Map::iterator
        iter = acceptor_map.begin ();
        iter!= acceptor_map.end ();
        iter++)
    {
        ACE_Reactor::instance()->suspend_handler((*iter).int_id_);
    }    
    return 0;
}

int SimService::resume ()
{
	SimAcceptor_Map& acceptor_map = SIM_MANAGER->getSimAcceptorMap();
    for( SimAcceptor_Map::iterator
        iter = acceptor_map.begin ();
        iter!= acceptor_map.end ();
        iter++)
    {
        ACE_Reactor::instance()->resume_handler((*iter).int_id_);
    }
    return 0;
}

ACE_FACTORY_DEFINE (ACE_Local_Service, SimService)

ACE_STATIC_SVC_DEFINE (
  SimService_Descriptor,
  ACE_TEXT ("SimService"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME (SimService),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0
)

ACE_STATIC_SVC_REQUIRE (SimService_Descriptor)
