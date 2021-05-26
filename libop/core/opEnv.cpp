#include "opEnv.h"
#include <windows.h>
void* opEnv::m_instance = nullptr;
std::wstring opEnv::m_basePath;
std::wstring opEnv::m_opName;
int opEnv::m_showErrorMsg = 1;
void opEnv::setInstance(void *instance)
{
    m_instance = instance;
     wchar_t buff[512]={};
    ::GetModuleFileNameW(static_cast<HINSTANCE>(m_instance),buff,512);
    std::wstring s(buff);
    size_t index =s.rfind(L"\\");
    if(index!=s.npos){
        m_basePath = s.substr(0,index);
        m_opName = s.substr(index + 1);
    }
}
void *opEnv::getInstance()
{
    return m_instance;
}

std::wstring opEnv::getBasePath(){
   
    return m_basePath;
}

std::wstring opEnv::getOpName(){
    return m_opName;
}