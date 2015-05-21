

#include "ace/Service_Config.h"
#include "ace/Service_Repository.h"
#include "ace/Service_Types.h"
#include "agent_service.h"
#include "ace/Get_Opt.h"
#include "ace/INET_Addr.h"
#include "log_msg.h"
#include "agent_ui.h"

AgentAcceptor::AgentAcceptor()
{
	MY_DEBUG( "AgentAcceptor::AgentAcceptor()\n" );
}

AgentAcceptor::~AgentAcceptor()
{
	MY_DEBUG( "AgentAcceptor::~AgentAcceptor()\n" );
}

int AgentAcceptor::handle_close (ACE_HANDLE, ACE_Reactor_Mask )
{
	MY_DEBUG( "AgentAcceptor::handle_close\n" );
	delete this;
	return 0;
}

int AgentService::init (int argc, ACE_TCHAR *argv[])
{
    MY_TRACE( ACE_TEXT ("AgentService::init\n") );
    
    ACE_NEW_RETURN(_acceptor, AgentAcceptor, -1);
    ACE_INET_Addr local_addr (AgentService::DEFAULT_PORT);
    
    // Start at argv[0].
    ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("p:u"), 0);
    get_opt.long_option (ACE_TEXT ("port"), 'p', 
                         ACE_Get_Opt::ARG_REQUIRED);
    
    bool launchUI = false;
    for (int c; (c = get_opt ()) != -1; )
    switch (c) {
    case 'p':
      local_addr.set_port_number
        (ACE_OS::atoi (get_opt.opt_arg ()));
      break;
    case 'u':
      launchUI = true;
      break;

    }
    
    if( _acceptor->open(local_addr) == -1 )
    {
        ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("AgentService::init")),
                      -1);
    }
    else
    {
        MY_NOTICE(ACE_TEXT("Agent is listening on IP: %s, Port: %d ...\n"),
                            local_addr.get_host_addr(),
                            local_addr.get_port_number());
    }

    if(launchUI)
    {
        AGENT_UI->start();
    }
    return 0;
}

int AgentService::fini ()
{
    MY_TRACE( ACE_TEXT ("AgentService::fini\n") );
    return 0;
}

int AgentService::info (ACE_TCHAR **, size_t) const
{
	return 0;
}

int AgentService::suspend ()
{
	return ACE_Reactor::instance()->suspend_handler(_acceptor);
}

int AgentService::resume ()
{
	return ACE_Reactor::instance()->resume_handler(_acceptor);
}

ACE_FACTORY_DEFINE (ACE_Local_Service, AgentService)

ACE_STATIC_SVC_DEFINE (
  AgentService_Descriptor,
  ACE_TEXT ("AgentService"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME (AgentService),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0
)

ACE_STATIC_SVC_REQUIRE (AgentService_Descriptor)
