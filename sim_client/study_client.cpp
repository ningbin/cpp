

#include "ace/Service_Config.h"
#include "ace/Service_Repository.h"
#include "ace/Service_Types.h"
#include "study_client.h"
#include "ace/Get_Opt.h"
#include "ace/INET_Addr.h"
#include "log_msg.h"

StudyConnector::StudyConnector():_stop(false)
{
	MY_DEBUG( "StudyConnector::StudyConnector()\n" );
}

StudyConnector::~StudyConnector()
{
	MY_DEBUG( "StudyConnector::~StudyConnector()\n" );
}

int StudyConnector::handle_close(ACE_HANDLE, ACE_Reactor_Mask )
{
	MY_DEBUG( "StudyConnector::handle_close()\n" );
	delete this;
	return 0;
}

int
StudyConnector::svc( void )
{
 	MY_DEBUG("StudyConnector::svc begin\n");
 	ACE_NEW_RETURN (_handler, StudyHandler(_tid), -1);
 	
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
  		ACE_NEW_RETURN (_handler, StudyHandler, -1);
  	}
  }
  MY_DEBUG("StudyConnector::svc end\n");
}

int StudyConnector::close (void)
{
	_stop = true;
	return ACE_Task_Base::close();
}

int StudyConnector::suspend (void)
{
	StudyConnectorT::suspend();
	return ACE_Task_Base::suspend();
}

int StudyConnector::resume (void)
{
	StudyConnectorT::resume();
	return ACE_Task_Base::resume();
}

int StudyClient::init (int argc, ACE_TCHAR *argv[])
{
	MY_TRACE( ACE_TEXT ("StudyClient::init begin\n") );
              
  ACE_NEW_RETURN(_connector, StudyConnector, -1);

	//Start at argv[0].
  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("i:p:t:"), 0);
  get_opt.long_option (ACE_TEXT ("ip"),
                       'i',
                       ACE_Get_Opt::ARG_REQUIRED);
  get_opt.long_option (ACE_TEXT ("port"),
                       'p',
                       ACE_Get_Opt::ARG_REQUIRED);
  get_opt.long_option (ACE_TEXT ("tid"),
                       't',
                       ACE_Get_Opt::ARG_REQUIRED);
	
	u_short port;
	ACE_TCHAR ip[MAXHOSTNAMELEN];
    ACE_TCHAR tid[MAXHOSTNAMELEN];
  for (int c; (c = get_opt ()) != -1; )
    switch (c) {
    case 'i':
      ACE_OS::strsncpy
        (ip, get_opt.opt_arg (), MAXHOSTNAMELEN);
     	break;
   	case 'p':
      port = static_cast<u_short> (ACE_OS::atoi (get_opt.opt_arg ()));;
      break;
    case 't':
      ACE_OS::strsncpy
        (tid, get_opt.opt_arg (), MAXHOSTNAMELEN);
     	break;
    }
	ACE_INET_Addr remote_addr(port,ip);
	
	_connector->set_remote_addr(remote_addr);
    ACE_CString s_tid = tid;
    _connector->setTid(s_tid);
	StudyHandler *handler;
 	ACE_NEW_RETURN (handler, StudyHandler(s_tid), -1);
	
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

int StudyClient::fini ()
{
	_connector->close();
	delete _connector;
	_connector = NULL;
  MY_TRACE( ACE_TEXT ("StudyClient::fini end\n") );

  return 0;
}

int StudyClient::info (ACE_TCHAR **, size_t) const
{
	
}

int StudyClient::suspend ()
{
	ACE_Reactor::instance()->suspend_handler((StudyConnectorT*)_connector);
}

int StudyClient::resume ()
{
	ACE_Reactor::instance()->resume_handler((StudyConnectorT*)_connector);
}

ACE_FACTORY_DEFINE (ACE_Local_Service, StudyClient)

ACE_STATIC_SVC_DEFINE (
  StudyClient_Descriptor,
  ACE_TEXT ("StudyClient"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME (StudyClient),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0
)

ACE_STATIC_SVC_REQUIRE (StudyClient_Descriptor)
