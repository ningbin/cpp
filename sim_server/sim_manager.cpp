
#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "log_msg.h"
#include "sim_manager.h"
#include "sim_handler.h"
#include "sim_service.h"
#include "tinyxml.h"

SimManager::SimManager()
{
    MY_DEBUG(ACE_TEXT("SimManager::SimManager()\n"));
}
SimManager::~SimManager()
{
    fini();
    MY_DEBUG(ACE_TEXT("SimManager::~SimManager()\n"));
}

int SimManager::fini( void )
{
    MY_DEBUG(ACE_TEXT("SimManager::fini\n"));
    
    _acceptor_map.unbind_all(); 
    deleteAllSimHandlerSet();
    deleteAllSimMaps();
    deleteAllSimNEs();

    return 0;
}

int SimManager::deleteAllSimHandlerSet()
{
    if(_handler_map.current_size()>0)
    {
        for( SimHandler_Map::iterator
        iter = _handler_map.begin ();
        iter!= _handler_map.end ();
        iter++)
        {
//          MY_DEBUG ("try to delete handler_set, key=%s\n",(*iter).ext_id_.c_str());
            SimHandlerSet* handler_set = (*iter).int_id_;
            if(handler_set)
            {
                MY_DEBUG ("delete handler_set, key=%s\n",(*iter).ext_id_.c_str());
                delete handler_set;
                handler_set = 0;
            }
        }
    }
    _handler_map.unbind_all();
    return 0;
}

int SimManager::deleteAllSimMaps()
{
    if(_map_map.current_size()>0)
    {
        for( SimMap_Map::iterator
        iter = _map_map.begin ();
        iter!= _map_map.end ();
        iter++)
        {
            MY_DEBUG ("try to delete sim_map, file=%s\n",(*iter).ext_id_.c_str());
            SimMap* sim_map = (*iter).int_id_;
            if(sim_map)
            {
                MY_DEBUG ("delete sim_map, file=%s\n",(*iter).ext_id_.c_str());
                delete sim_map;
                sim_map = 0;
            }
        }
    }
    _map_map.unbind_all();
    return 0;
}

int SimManager::deleteAllSimNEs(void)
{
    if(_ne_map.current_size()>0)
    {
        for( SimNE_Map::iterator
        iter = _ne_map.begin ();
        iter!= _ne_map.end ();
        iter++)
        {
//          MY_DEBUG ("try to delete NE, tid=%s\n",(*iter).ext_id_.c_str());
            SimNE* ne = (*iter).int_id_;
            if(ne)
            {
                MY_DEBUG ("delete NE, tid=%s\n",(*iter).ext_id_.c_str());
                delete ne;
                ne = 0;
            }
        }
    }
    _ne_map.unbind_all();
    return 0;
}

int SimManager::addSimHandler(const ACE_CString& key, SimHandler *handler)
{
    MY_DEBUG(ACE_TEXT("SimManager::addSimHandler, key=%s\n"), key.c_str());
    SimHandlerSet* handler_set;
    if(_handler_map.find( key, handler_set )==-1)
    {
        handler_set = new SimHandlerSet;
        handler_set->insert(handler);
        return _handler_map.bind(key, handler_set);
    }
    else
    {
        handler_set->insert(handler);
        return 0;
    }
}

int SimManager::addSimAcceptor(const ACE_CString& key, SimAcceptor *acceptor)
{
    MY_DEBUG(ACE_TEXT("SimManager::addSimAcceptor, key=%s\n"), key.c_str());
    return _acceptor_map.bind( key, acceptor );
}

int SimManager::removeSimHandler(const ACE_CString& key, SimHandler *handler)
{
    MY_DEBUG(ACE_TEXT("SimManager::removeSimHandler, key=%s\n"), key.c_str());
    SimHandlerSet* handler_set;
    if(_handler_map.find( key, handler_set )==-1)
    {
        return -1;
    }
    else if(handler_set)
    {
        handler_set->remove(handler);
        if(handler_set->is_empty())
        {
//          MY_DEBUG ("remove handler_set, key=%s\n",key.c_str());
            delete handler_set;
            handler_set = 0;
            _handler_map.unbind(key);
        }
    }
    return 0;
}

int SimManager::removeSimAcceptor( const ACE_CString& key, SimAcceptor *acceptor )
{
    MY_DEBUG(ACE_TEXT("SimManager::removeSimAcceptor, key=%s\n"), key.c_str());
    if(acceptor)
    {
        _acceptor_map.unbind( key, acceptor );   
    }
    else
    {
        _acceptor_map.unbind( key );
    }
    return 0;
}

SimHandlerSet* SimManager::getSimHandlerSet(const ACE_CString& key)
{
    SimHandlerSet* handler_set = 0;
    if(_handler_map.find(key, handler_set)==-1)
    {
        MY_DEBUG("Cannot find handler list for %s\n",key.c_str());
    }
    return handler_set;
}

int SimManager::findSimHandler(const ACE_CString& key, SimHandler* handler)
{
    SimHandlerSet* handler_set = 0;
    if(_handler_map.find(key, handler_set)==-1)
    {
        return -1;
    }
    if(handler_set)
    {
        if( handler_set->find(handler) != -1 ) ;
        {
            return 0;
        }
    }
    return -1;
}

SimAcceptor* SimManager::getSimAccetpor(const ACE_CString& key)
{
    SimAcceptor* accetpor = 0;
    if(_acceptor_map.find(key, accetpor)==-1)
    {
        MY_DEBUG("Cannot find accetpor for %s\n",key.c_str());
    }
    return accetpor;
}

SimMap* SimManager::getSimMap(const ACE_CString& file)
{
    SimMap* sim_map = 0;
    if(_map_map.find(file, sim_map)==-1)
    {
        MY_DEBUG("Cannot find sim_map for %s\n",file.c_str());
    }
    return sim_map;
}

int SimManager::createSimMap(const ACE_CString& file )
{
    MY_DEBUG("create sim_map, file=%s\n", file.c_str());
    SimMap* sim_map = 0;
    if(_map_map.find(file, sim_map)!=-1)
    {
        if(sim_map)
        {
            MY_DEBUG("sim_map already exists, file=%s\n",file.c_str());
            sim_map->increaseCnt();
        }
        return 0;
    }

    ACE_NEW_RETURN(sim_map, SimMap(file), -1);
    if( sim_map->init()==-1 )
    {
        return -1;
    }
    else
    {
        _map_map.bind( file, sim_map );
        sim_map->increaseCnt();
    }
    return 0;
}

int SimManager::deleteSimMap(const ACE_CString& file )
{
    MY_DEBUG("delete sim_map, file=%s\n",file.c_str());
    SimMap* sim_map = 0;
    if( _map_map.find(file, sim_map)<0 )
    {
        MY_DEBUG("Cannot find sim_map, file=%s\n",file.c_str());
        return -1;
    }
    if(sim_map)
    {
        sim_map->decreaseCnt();
        if( sim_map->tryDelete() == 0 )
        {
            _map_map.unbind( file, sim_map );
        }
        return 0;
    }
    else
    {
        _map_map.unbind( file );
    }
    return 0;
}

SimNE::SimNE(const ACE_CString& tid, const ACE_CString& type, 
             const ACE_CString& gne, const ACE_CString& conf,
             int port, SimManager* manager, const ACE_CString& tidIpMap )
        :_tid(tid),_type(type),_gne(gne),_conf(conf),_port(port),
        _sim_map(0),_manager(manager),_tidIpMap(tidIpMap)
{ 
    MY_DEBUG("SimNE::SimNE(), tid=%s\n",_tid.c_str());
}

SimNE::~SimNE()
{
    MY_DEBUG("SimNE::~SimNE(), tid=%s\n",_tid.c_str());
}

SimMap* SimNE::getSimMap()
{
    if(_sim_map==NULL)
    {
        _sim_map = this->getManager()->getSimMap(_conf);
    }
    return _sim_map;
}

int SimManager::createSimNE(const ACE_CString& tid, const ACE_CString& type, 
                            const ACE_CString& gne, const ACE_CString& conf,
                            int port, const ACE_CString& tidIpMap)
{
    MY_DEBUG("create NE, tid=%s\n", tid.c_str());
    SimNE* ne = 0;
    ACE_NEW_RETURN(ne, SimNE(tid,type,gne,conf,port,this, tidIpMap), -1);

    _ne_map.bind( tid, ne );
    return 0;
}

int SimManager::deleteSimNE(const ACE_CString& tid )
{
    MY_DEBUG("delete NE, tid=%s\n",tid.c_str());
    SimNE* ne = 0;
    if(_ne_map.find(tid, ne)==-1)
    {
        MY_DEBUG("Cannot find NE, tid=%s\n",tid.c_str());
        return -1;
    }
    if(ne)
    {
        _ne_map.unbind( tid, ne );
        delete ne;
        ne = 0;
        return 0;
    }
    else
    {
        _ne_map.unbind( tid );
    }
    return 0;
}

SimNE* SimManager::getSimNE(const ACE_CString& tid)
{
    SimNE* ne = 0;
    if(_ne_map.find(tid, ne)==-1)
    {
        MY_DEBUG("Cannot find NE, tid=%s\n",tid.c_str());
    }
    return ne;
}

int SimManager::getTidAndConfList(const ACE_CString& tid,
                                  std::list<ACE_CString>& tidList, std::list<ACE_CString>& confList)
{
    for( SimNE_Map::iterator
        iter = _ne_map.begin ();
        iter!= _ne_map.end ();
        iter++)
    {
        SimNE* ne = (*iter).int_id_;
        if( ne && ne->_gne==tid )
        {
            tidList.push_back(ne->_tid);
            confList.push_back(ne->_conf);
        }
    }
    
    return 0;
}


int SimManager::startAllNE(const ACE_TCHAR* conf)
{
    ACE_FILE_Connector connector;
    ACE_FILE_Info fileinfo;
    ACE_FILE_IO file;
    ACE_CString name = conf;
    ACE_FILE_Addr fileaddr(name.c_str());

    if (connector.connect (file, fileaddr) == -1)
    {
        MY_ERROR(ACE_TEXT("cannot open file: %s\n"),name.c_str());
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

        for( TiXmlElement *neElement = 
             doc.FirstChildElement( "CONFIGURATION" )->FirstChildElement("NE");
             neElement;
             neElement = neElement->NextSiblingElement("NE") )
        {
            ACE_CString ne_tid, ne_conf, ne_type;
            int ne_port=0;
            TiXmlElement* tidElement = neElement->FirstChildElement( "tid" );
            if(tidElement)
            {
                ne_tid = tidElement->GetText();
                MY_DEBUG(ACE_TEXT("tid is %s\n"), ne_tid.c_str());
            }
            TiXmlElement* portElement = neElement->FirstChildElement( "port" );
            if(portElement)
            {
                ACE_CString s_port = portElement->GetText();
                ne_port = ACE_OS::atoi(s_port.c_str());
                MY_DEBUG(ACE_TEXT("port is %d\n"), ne_port);
            }
            TiXmlElement* confElement = neElement->FirstChildElement( "conf" );
            if(confElement)
            {
                ne_conf = confElement->GetText();
                MY_DEBUG(ACE_TEXT("conf is %s\n"), ne_conf.c_str());
            }
            TiXmlElement* typeElement = neElement->FirstChildElement( "type" );
            if(typeElement)
            {
                ne_type = typeElement->GetText();
                MY_DEBUG(ACE_TEXT("type is %s\n"), ne_type.c_str());
            }

            if( ne_type=="standalone" || ne_type=="gne" )
            {
                if( ne_tid.empty() )
                {
                    MY_ERROR("missing attribute \"tid\"\n" );
                    return -1;
                }
                if( ne_conf.empty() )
                {
                    MY_ERROR("missing attribute \"conf\"\n" );
                    return -1;
                }
                if( ne_port==0 )
                {
                    MY_ERROR("missing attribute \"port\"\n" );
                    return -1;
                }
                if( ne_type=="standalone" && 
                    startNE(ne_tid, ne_type, ne_tid, ne_conf, ne_port )==-1 )
                {
                    MY_ERROR("start NE %s failed! port=%d, conf=%s\n",
                             ne_tid.c_str(), ne_port, ne_conf.c_str() );
                    return -1;
                }
            }
            else
            {
                MY_ERROR("NE type error! type = %s\n", ne_type.c_str() );
                return -1;
            }
            if( ne_type=="gne" )
            {
                ACE_CString tidIpMap;
                for( TiXmlElement *rneElement = 
                     neElement->FirstChildElement("rne");
                     rneElement;
                     rneElement = rneElement->NextSiblingElement("rne") )
                {
                    ACE_CString rne_tid, rne_conf;
                    TiXmlElement* rneTidElement = rneElement->FirstChildElement( "tid" );
                    if(rneTidElement)
                    {
                        rne_tid = rneTidElement->GetText();
                        MY_DEBUG(ACE_TEXT("rne tid is %s\n"), rne_tid.c_str());
                    }
                    TiXmlElement* rneConfElement = rneElement->FirstChildElement( "conf" );
                    if(rneConfElement)
                    {
                        rne_conf = rneConfElement->GetText();
                        MY_DEBUG(ACE_TEXT("rne conf is %s\n"), rne_conf.c_str());
                    }
                    if(rne_tid.empty())
                    {
                        MY_ERROR("missing attribute \"rnc tid\"\n" );
                        return -1;
                    }
                    if(rne_conf.empty())
                    {
                        MY_ERROR("missing attribute \"rnc conf\"\n" );
                        return -1;
                    }
                    if( startNE(rne_tid, "rne", ne_tid, rne_conf, ne_port )==-1 )
                    {
                        MY_ERROR("start RNE %s failed! tid=%s, conf=%s\n",
                                 rne_tid.c_str(), rne_conf.c_str() );
                        return -1;
                    }
                    tidIpMap += "\n\"::RNETID=\\\"";
                    tidIpMap += rne_tid;
                    tidIpMap += "\\\",RNEIP=0.0.0.1,MODE=DYNAMIC\"";
                }

                if( startNE(ne_tid, ne_type, ne_tid, ne_conf, ne_port, tidIpMap )==-1 )
                {
                    MY_ERROR("start GNE %s failed! port=%d, conf=%s\n",
                             ne_tid.c_str(), ne_port, ne_conf.c_str() );
                    return -1;
                }
            }
        }
    }
    return 0;
}

int SimManager::startNE( const ACE_CString& tid, const ACE_CString& type, 
                         const ACE_CString& gne, const ACE_CString& conf, 
                         int port, const ACE_CString& tidIpMap)
{
    if(type!="rne")
    {
        ACE_INET_Addr local_addr;
        local_addr.set_port_number(port);
    
        SimAcceptor* acceptor;
        ACE_NEW_RETURN(acceptor, SimAcceptor(tid, port), -1);
        if( acceptor->open(local_addr) == -1 )
        {
            ACE_ERROR_RETURN ((LM_ERROR,
                               ACE_TEXT ("%p\n"),
                               ACE_TEXT ("SimService::startNE")),
                              -1);
        }
        else
        {
            MY_NOTICE(ACE_TEXT("NE %s is listening on IP: %s, Port: %d ...\n"),
                            tid.c_str(),
                            local_addr.get_host_addr(),
                            local_addr.get_port_number());
            this->addSimAcceptor(tid, acceptor);        
        }
    }
    this->createSimMap(conf);
    this->createSimNE(tid,type,gne,conf,port,tidIpMap);

    return 0;
}

