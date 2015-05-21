
#ifndef __AGENT_REQUEST_H_
#define __AGENT_REQUEST_H_

#include <map>
#include <list>
#include <ace/SString.h>

struct request_t
{
    ACE_CString id;
    ACE_CString action;
    std::map<ACE_CString, ACE_CString> input;
};

typedef std::list<request_t> bulk_request_t;

class AgentRequest
{
    public:
        int fromXml(const ACE_TCHAR *xml, ACE_TCHAR type);
        
        ACE_CString _version;
        ACE_CString _time;
        request_t _request;
        bulk_request_t _bulk_request;

    private:
        ACE_TCHAR _type;
};

#endif /* __AGENT_REQUEST_H_ */

