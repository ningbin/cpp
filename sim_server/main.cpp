// $Id: main.cpp 91670 2012-11-02 11:08:26Z bning $

#include "ace/OS_NS_unistd.h"
#include "ace/Service_Config.h"
#include "ace/Reactor.h"
#include "ace/Signal.h"
#include <ace/Thread_Manager.h>
#include <ace/TP_Reactor.h>
#include "log_msg.h"

const int default_reactor_threads = 5;

class Quit_Handler : public ACE_Event_Handler
{
    public:
        Quit_Handler (ACE_Reactor *r) : ACE_Event_Handler (r) {}
        
        virtual int handle_signal(int signo, siginfo_t * = 0, ucontext_t * = 0) 
        {
            MY_DEBUG("Quit_Handler::handle_signal, signo=%d\n", signo);
            reactor()->end_reactor_event_loop ();
            return -1; // Trigger call to handle_close() method.
        }
        
        virtual int handle_close (ACE_HANDLE, ACE_Reactor_Mask)
        { 
            //MY_DEBUG("Quit_Handler::handle_close\n");
            delete this; 
            return 0; 
        }
    
    protected:
    
        // Protected destructor ensures dynamic allocation.
        virtual ~Quit_Handler ()
        {
            //MY_DEBUG("~Quit_Handler()\n");
        }
};

ACE_THR_FUNC_RETURN event_loop(void *arg)
{
    ACE_Reactor *reactor = static_cast<ACE_Reactor *> (arg);
    
    reactor->owner (ACE_OS::thr_self ());
    reactor->run_reactor_event_loop ();
    return 0;
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
    ACE_TP_Reactor tpReactor;
    ACE_Reactor reactor(&tpReactor);
    ACE_Reactor::instance (&reactor);
    
    if (ACE_Service_Config::open (argc,
                              argv,
                              ACE_DEFAULT_LOGGER_KEY,
                              0) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                     ACE_TEXT ("%p\n"),
                     ACE_TEXT ("ACE_Service_Config::open")),
                    -1);
                    
    MY_TRACE( ACE_TEXT ("Main begin\n"));
    
    Quit_Handler *quit_handler = 0;
    ACE_NEW_RETURN (quit_handler, Quit_Handler(ACE_Reactor::instance()), 0);
    
    ACE_Sig_Set sig_set;
    sig_set.sig_add (SIGINT);
    sig_set.sig_add (SIGQUIT);
    
    
    if (ACE_Reactor::instance()->register_handler (sig_set, quit_handler) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"), ACE_TEXT ("register_handler")),
                      -1);
    
    // spawn some threads which run the reactor event loop(s)
    int grp_id = ACE_Thread_Manager::instance()->spawn_n(default_reactor_threads, event_loop, &reactor);
    
    // let the thread manager wait for all threads
    ACE_Thread_Manager::instance()->wait_grp(grp_id);
    
    MY_TRACE( ACE_TEXT ("Main end\n") );
	
    return 0;
}
