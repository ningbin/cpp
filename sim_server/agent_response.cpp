#include <ace/Message_Block.h>
#include "log_msg.h"
#include "tinyxml.h"
#include "agent_response.h"


ACE_Message_Block* AgentResponse::toXml()
{
    TiXmlDocument doc; 
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );  
    doc.LinkEndChild( decl );  

    TiXmlElement * root = new TiXmlElement( "Message" );  
    doc.LinkEndChild( root );  

    TiXmlElement * head = new TiXmlElement( "Head" );  
    root->LinkEndChild( head );  

    TiXmlElement * version = new TiXmlElement( "Version" );  
    version->LinkEndChild( new TiXmlText( _version.c_str() ));  
    head->LinkEndChild( version );  

    TiXmlElement * time = new TiXmlElement( "Time" ); 
    ACE_TCHAR day_and_time[27];
    ACE::timestamp (day_and_time, sizeof day_and_time);
    time->LinkEndChild( new TiXmlText( day_and_time ));  
    head->LinkEndChild( time );

    TiXmlElement * body = new TiXmlElement( "Body" );  
    root->LinkEndChild( body );  

    TiXmlElement * response = new TiXmlElement( "Response" );  
    body->LinkEndChild( response );  

    TiXmlElement * body_id = new TiXmlElement( "ReqID" );
    body_id->LinkEndChild( new TiXmlText( _response.req_id.c_str() ));
    response->LinkEndChild( body_id ); 

    TiXmlElement * body_action = new TiXmlElement( "Action" );
    body_action->LinkEndChild( new TiXmlText( _response.action.c_str() ));  
    response->LinkEndChild( body_action ); 

    TiXmlElement * body_result = new TiXmlElement( "Result" );
    body_result->LinkEndChild( new TiXmlText( _response.result.c_str() ));  
    response->LinkEndChild( body_result );
    
    TiXmlElement * body_output = new TiXmlElement( "Output" );
    response->LinkEndChild( body_output );

    TiXmlElement * body_exception = new TiXmlElement( "Exception" );
    body_exception->LinkEndChild( new TiXmlText( _response.except.c_str() ));  
    response->LinkEndChild( body_exception ); 
    
    for(std::map<ACE_CString, ACE_CString>::iterator
         it = this->_response.output.begin();
         it!= this->_response.output.end();
         it++ )
    {
        TiXmlElement * element = new TiXmlElement( "Element" );
        element->SetAttribute( "name", (it->first).c_str() );
        element->LinkEndChild( new TiXmlText( (it->second).c_str() ));  
        body_output->LinkEndChild( element );
    }

    TiXmlPrinter printer;  
    doc.Accept( &printer );

    ACE_UINT32 length = printer.Size();
    MY_DEBUG(ACE_TEXT("msg size = %u\n"), length);

    ACE_UINT32 net_len = ACE_HTONL(length);
    ACE_TCHAR type = 0x02;

//  MY_DEBUG( ACE_TEXT ("response=>\n%.*C"),
//          static_cast<int> (length),
//          printer.CStr());

    ACE_UINT32 data_len = sizeof(ACE_UINT32)+sizeof(ACE_TCHAR)+length;		
    ACE_Message_Block *mb = new ACE_Message_Block(data_len);

    ACE_OS::memcpy (mb->wr_ptr(), &net_len, sizeof(ACE_UINT32));
    mb->wr_ptr(sizeof(ACE_UINT32));

    ACE_OS::memcpy (mb->wr_ptr(), &type, sizeof(ACE_TCHAR));
    mb->wr_ptr(sizeof(ACE_TCHAR));

    ACE_OS::memcpy (mb->wr_ptr(), printer.CStr(), length);
    mb->wr_ptr(length);

    return mb;
}
