
#ifndef __LOG_MSG_H_
#define __LOG_MSG_H_

#include "ace/Log_Msg.h"

#define DEBUG_PREFIX       ACE_TEXT ("%D(%P|%t)<%M>%N:%l:") 
#define TRACE_PREFIX       ACE_TEXT ("%D(%P|%t)<%M>%N:%l:")
#define INFO_PREFIX        ACE_TEXT ("%D(%P|%t)<%M>")
#define NOTICE_PREFIX      ACE_TEXT ("%D(%P|%t)<%M>")
#define WARNING_PREFIX     ACE_TEXT ("%D(%P|%t)<%M>")
#define ERROR_PREFIX       ACE_TEXT ("%D(%P|%t)<%M>%N:%l:")
#define CRITICAL_PREFIX    ACE_TEXT ("%D(%P|%t)<%M>")

#define MY_DEBUG(...)     \
        ACE_DEBUG(( LM_DEBUG,  \
                    DEBUG_PREFIX __VA_ARGS__))
#define MY_TRACE(...)     \
        ACE_DEBUG(( LM_TRACE,  \
                    TRACE_PREFIX __VA_ARGS__))              
#define MY_INFO(...)     \
        ACE_DEBUG(( LM_INFO,  \
                    INFO_PREFIX __VA_ARGS__))
#define MY_NOTICE(...)     \
        ACE_DEBUG(( LM_NOTICE,  \
                    NOTICE_PREFIX __VA_ARGS__))
#define MY_WARNING(...)     \
        ACE_DEBUG(( LM_WARNING,  \
                    WARNING_PREFIX __VA_ARGS__))
#define MY_ERROR(...)     \
        ACE_DEBUG(( LM_ERROR,  \
                    ERROR_PREFIX  __VA_ARGS__))
#define MY_CRITICAL(...)     \
        ACE_DEBUG(( LM_CRITICAL,  \
                    CRITICAL_PREFIX __VA_ARGS__))

#endif /* __LOG_MSG_H_ */
