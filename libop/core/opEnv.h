#ifndef __OPENV_H_
#define __OPENV_H_
#include <string>
class opEnv
{
public:
    static void setInstance(void *instance);
    static void *getInstance();
    static std::wstring getBasePath();
    static std::wstring getOpName();
    static int m_showErrorMsg;
private:
    static void *m_instance;
    static std::wstring m_basePath;
    static std::wstring m_opName;
    
};
#endif