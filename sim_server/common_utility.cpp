#include "common_utility.h"

ACE_CString CommonUtility::str_int(int num)
{
    ACE_CString result;
    ACE_TCHAR tmp[sizeof(int)+1];
    ACE_OS::memset(tmp,0,sizeof tmp);
    ACE_OS::sprintf(tmp,"%d",num);
    result = tmp;
    return result;
}

ACE_CString CommonUtility::str_size_t(size_t num)
{
    ACE_CString result;
    ACE_TCHAR tmp[sizeof(size_t)+1];
    ACE_OS::memset(tmp,0,sizeof tmp);
    ACE_OS::sprintf(tmp,"%d",num);
    result = tmp;
    return result;
}

ACE_CString CommonUtility::str_uint64(ACE_UINT64 num)
{
    ACE_CString result;
    ACE_TCHAR tmp[sizeof(ACE_UINT64)+1];
    ACE_OS::memset(tmp,0,sizeof tmp);
    ACE_OS::sprintf(tmp,"%d",num);
    result = tmp;
    return result;
}

ACE_CString CommonUtility::str_double(double num, const ACE_TCHAR* format)
{
    ACE_CString result;
    ACE_TCHAR tmp[ sizeof(double) + 1];
    ACE_OS::memset(tmp,0,sizeof(tmp));
    ACE_OS::sprintf(tmp,format,num);
    result  = tmp;
    return result;
}

void CommonUtility::tokenize(std::vector<ACE_CString>& tokens, const ACE_TCHAR* cmd, ACE_TCHAR delimiter, ACE_TCHAR ender )
{
    ACE_CString tmp(cmd);
    size_t i;
    ACE_TCHAR c;
    while( (i = tmp.find(delimiter)) != ACE_CString::npos )
    {
    	ACE_CString token;
    	if(i == 0)
    	{
    		 token = "";   		
    	}
    	else
    	{
	    	token = tmp.substr(0,i);	    	
	    }
	    tokens.push_back(token);
    	tmp = tmp.substr(i+1);
    }
    
    c = tmp[tmp.length()-1];
    if( c==ender )
    {
        tmp = tmp.substr(0,tmp.length()-1);
    }

    tokens.push_back(tmp);
}
