
#ifndef __AGENT_RESPONSE_H_
#define __AGENT_RESPONSE_H_

#include <map>
#include <list>
#include <ace/SString.h>

struct response_t
{
    ACE_CString req_id;
    ACE_CString action;
    ACE_CString result;
    ACE_CString except;
    std::map<ACE_CString, ACE_CString> output;
};

typedef std::list<response_t> bulk_response_t;

class AgentResponse
{
    public:
        ACE_Message_Block* toXml();
        
        ACE_CString _version;
        response_t _response;
        bulk_response_t _bulk_response;

    private:
        ACE_TCHAR _type;
};

#endif /* __AGENT_RESPONSE_H_ */

