#include "log_msg.h"
#include "tinyxml.h"
#include "agent_request.h"

int AgentRequest::fromXml(const ACE_TCHAR *xml, ACE_TCHAR type)
{
    _type = type;
//  MY_DEBUG("request type = %d\n",_type);

    TiXmlDocument doc;
    doc.Parse( xml );
    if (true == doc.Error())
    {
        MY_ERROR(ACE_TEXT("XML parse error!!! row: %d, col: %d, error: %s\n"),
                            doc.ErrorRow(),
                            doc.ErrorCol(),
                            doc.ErrorDesc());
        return -1;
    }
    
    TiXmlElement* head = doc.FirstChildElement( "Message" )->FirstChildElement( "Head" );
    TiXmlElement* version = head->FirstChildElement( "Version" );
    if(version)
    {
        _version = version->GetText();
//      MY_DEBUG(ACE_TEXT("request version is %s\n"), _version.c_str());
    }
    TiXmlElement* time = head->FirstChildElement( "Time" );
    if(time)
    {
        _time = time->GetText();
//      MY_DEBUG(ACE_TEXT("request time is %s\n"), _time.c_str());
    }
    
    TiXmlElement* request = doc.FirstChildElement( "Message" )->FirstChildElement( "Body" )->FirstChildElement( "Request" );
    TiXmlElement* req_id = request->FirstChildElement( "ID" );
    if(req_id)
    {
        _request.id = req_id->GetText();
//      MY_DEBUG(ACE_TEXT("request id is %s\n"), _request.id.c_str());
    }
    TiXmlElement* req_action = request->FirstChildElement( "Action" );
    if(req_action)
    {
        _request.action = req_action->GetText();
//      MY_DEBUG(ACE_TEXT("request action is %s\n"), _request.action.c_str());
    }
    
    TiXmlElement* req_input = request->FirstChildElement( "Input" );
    if(req_input)
    {
        for( TiXmlElement* input_element = req_input->FirstChildElement( "Element" );
         input_element;
         input_element = input_element->NextSiblingElement("Element") )
        {
            ACE_CString name = input_element->Attribute("name" );
            ACE_CString value = input_element->GetText();
            _request.input.insert(std::pair<ACE_CString,ACE_CString>(name, value));
//          MY_DEBUG(ACE_TEXT("request input name is %s, value is %s\n"), name.c_str(), value.c_str());
        }
    }
    return 0;
}
