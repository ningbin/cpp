

#include "ace/Service_Config.h"
#include "ace/Service_Repository.h"
#include "ace/Service_Types.h"
#include "agent_client.h"
#include "ace/Get_Opt.h"
#include "ace/INET_Addr.h"
#include "log_msg.h"

AgentConnector::AgentConnector():_stop(false)
{
	MY_DEBUG( "AgentConnector::AgentConnector()\n" );
}

AgentConnector::~AgentConnector()
{
	MY_DEBUG( "AgentConnector::~AgentConnector()\n" );
}

int AgentConnector::handle_close(ACE_HANDLE, ACE_Reactor_Mask )
{
	MY_DEBUG( "AgentConnector::handle_close()\n" );
	delete this;
	return 0;
}

int
AgentConnector::svc( void )
{
 	MY_DEBUG("AgentConnector::svc begin\n");
 	ACE_NEW_RETURN (_handler, AgentHandler, -1);
 	
 	bool connected = false;
 	ACE_Time_Value timeout (3);
 	ACE_Synch_Options options (ACE_Synch_Options::USE_TIMEOUT,
                               timeout);
 	while( !_stop )
  {
  	if(this->connect(_handler, this->get_remote_addr(), options ) != -1)
  	{
  		connected = true;
  		MY_DEBUG("Connected!!\n");
  		break;
  	}
  	else
  	{
  		MY_DEBUG("Cannot connect to server, wait...\n");
  		ACE_OS::sleep (timeout);
  		ACE_NEW_RETURN (_handler, AgentHandler, -1);
  	}
  }
  MY_DEBUG("AgentConnector::svc end\n");
}

int AgentConnector::close (void)
{
	_stop = true;
	return ACE_Task_Base::close();
}

int AgentConnector::suspend (void)
{
	AgentConnectorT::suspend();
	return ACE_Task_Base::suspend();
}

int AgentConnector::resume (void)
{
	AgentConnectorT::resume();
	return ACE_Task_Base::resume();
}

int AgentClient::init (int argc, ACE_TCHAR *argv[])
{
	MY_TRACE( ACE_TEXT ("AgentClient::init begin\n") );
              
  ACE_NEW_RETURN(_connector, AgentConnector, -1);

	//Start at argv[0].
  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("i:p:"), 0);
  get_opt.long_option (ACE_TEXT ("ip"),
                       'i',
                       ACE_Get_Opt::ARG_REQUIRED);
  get_opt.long_option (ACE_TEXT ("port"),
                       'p',
                       ACE_Get_Opt::ARG_REQUIRED);
	
	u_short port;
	ACE_TCHAR ip[MAXHOSTNAMELEN];
  for (int c; (c = get_opt ()) != -1; )
    switch (c) {
    case 'i':
      ACE_OS::strsncpy
        (ip, get_opt.opt_arg (), MAXHOSTNAMELEN);
     	break;
   	case 'p':
      port = static_cast<u_short> (ACE_OS::atoi (get_opt.opt_arg ()));;
      break;
    }	
	ACE_INET_Addr remote_addr(port,ip);
	
	_connector->set_remote_addr(remote_addr);
	AgentHandler *handler;
 	ACE_NEW_RETURN (handler, AgentHandler, -1);
	
	if(_connector->connect(handler, remote_addr) == -1)
	{
		_connector->activate();
	}
	else
	{
		_connector->setHandler(handler);
		MY_DEBUG("Connected!!\n");
	}
	return 0;
}

int AgentClient::fini ()
{
	_connector->close();
	delete _connector;
	_connector = NULL;
  MY_TRACE( ACE_TEXT ("AgentClient::fini end\n") );

  return 0;
}

int AgentClient::info (ACE_TCHAR **, size_t) const
{
	
}

int AgentClient::suspend ()
{
	ACE_Reactor::instance()->suspend_handler((AgentConnectorT*)_connector);
}

int AgentClient::resume ()
{
	ACE_Reactor::instance()->resume_handler((AgentConnectorT*)_connector);
}

ACE_FACTORY_DEFINE (ACE_Local_Service, AgentClient)

ACE_STATIC_SVC_DEFINE (
  AgentClient_Descriptor,
  ACE_TEXT ("AgentClient"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME (AgentClient),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0
)

ACE_STATIC_SVC_REQUIRE (AgentClient_Descriptor)
