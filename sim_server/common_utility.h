
#ifndef __COMMON_UTILITY_H_
#define __COMMON_UTILITY_H_

#include "ace/SString.h" 
#include <vector>

class CommonUtility
{
    public:
        static ACE_CString str_int(int num);
        static ACE_CString str_size_t(size_t num);
        static ACE_CString str_uint64(ACE_UINT64 num);
        static ACE_CString str_double(double num, const ACE_TCHAR* format);

        static void tokenize(std::vector<ACE_CString>& tokens, const ACE_TCHAR* cmd, ACE_TCHAR delimiter=':', ACE_TCHAR ender=';' );
};

#endif /* __COMMON_UTILITY_H_ */

