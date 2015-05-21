
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "log_msg.h"
#include "sim_map.h"
#include "tinyxml.h"

SimMap::SimMap(const ACE_CString& file): _file(file),_cnt(0)
{
    MY_DEBUG(ACE_TEXT("SimMap::SimMap(), file=%s\n"), _file.c_str());
}
SimMap::~SimMap()
{
    MY_DEBUG(ACE_TEXT("SimMap::~SimMap(), file=%s\n"), _file.c_str());
    fini();
}

int SimMap::init( void )
{
    ACE_FILE_Connector connector;
    ACE_FILE_Info fileinfo;
    ACE_FILE_IO file;
    ACE_CString name = _file;
    ACE_FILE_Addr fileaddr(name.c_str());

    if (connector.connect (file, fileaddr) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot open file: %s\n"),name.c_str());
        return -1;
    }
    else
    {
        MY_DEBUG(ACE_TEXT("open file: %s\n"),name.c_str());
        if (file.get_info (&fileinfo) == -1)
        {
            MY_ERROR(ACE_TEXT("cannot get fileinfo of  %s\n"), name.c_str());
            return -1;
        }
        else
        {
//          MY_DEBUG(ACE_TEXT("file size = %u\n"),fileinfo.size_);
        }
        ACE_TCHAR *buf = new ACE_TCHAR[fileinfo.size_];
        u_long rbytes;
        if ((rbytes = file.recv_n (buf, fileinfo.size_ )) != fileinfo.size_)
        {
            MY_ERROR(ACE_TEXT("cannot read file %s correctly!\n"), name.c_str());
            return -1;
        }
        
        TiXmlDocument doc;
        doc.Parse( buf );
        delete[] buf;
        if (true == doc.Error())
        {
            MY_ERROR(ACE_TEXT("XML parse error!!! file: %s, row: %d, col: %d, error: %s\n"), 
                                name.c_str(), 
                                doc.ErrorRow(),
                                doc.ErrorCol(),
                                doc.ErrorDesc());
            return -1;
        }

        TiXmlElement *element = doc.FirstChildElement( "CONFIGURATION" )->FirstChildElement();
        for(;element;element=element->NextSiblingElement())
        {
            ACE_CString key = element->Value();
            ACE_CString value = element->GetText();
            value = BeautifyText(value);
            //MY_DEBUG(ACE_TEXT("\n\nKey:%s\nValue:\n%s\n"),key.c_str(),value.c_str());
            _map.bind(key,value);
        }
    }
    MY_DEBUG(ACE_TEXT("SimMap::init end\n"));
    return 0;
}

int SimMap::suspend( void )
{
    return fini();
}

int SimMap::resume( void )
{
    return init();
}

int SimMap::fini( void )
{
    _map.unbind_all();
    return 0;
}

int SimMap::getValue(const ACE_CString& key, ACE_CString& value)
{
    return _map.find(key, value);
}

ACE_CString SimMap::BeautifyText(const ACE_CString& value)
{
    ACE_CString result;
    size_t leng = value.length();
    if( leng <=0 )
        return result;
    ACE_TCHAR *buf = new ACE_TCHAR[leng+1];
    
    size_t cur = 0;
    for( ; cur<leng; cur++)
    {
        if( value[cur]==' ' &&
            ( cur>0 && value[cur-1]=='\"' ) && 
            ( cur<leng-1 && value[cur+1]=='\"') )
        {
            buf[cur]='\n';
        }
        else
        {
            buf[cur]=value[cur];
        }
    }
    buf[cur] = '\0';

    result = buf;
    delete[] buf;
    return result;
}

int SimMap::increaseCnt()
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, _lock, -1);
    ++_cnt;
    MY_DEBUG("SimMap::increaseCnt, _cnt=%d\n",_cnt);
    return _cnt;
}

int SimMap::decreaseCnt()
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, _lock, -1);
    --_cnt;
    MY_DEBUG("SimMap::decreaseCnt, _cnt=%d\n",_cnt);
    return _cnt;
}

int SimMap::tryDelete()
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, _lock, -1);
    
    if(_cnt>0)
    {
        MY_DEBUG("SimMap::tryDelete, _cnt=%d, not deleted!\n",_cnt);
        return -1;
    }
    MY_DEBUG("SimMap::tryDelete, _cnt=%d, deleted!\n",_cnt);
    delete this;
    return 0;
}
