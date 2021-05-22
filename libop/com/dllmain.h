// dllmain.h: 模块类的声明。
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
class CopModule : public ATL::CAtlDllModuleT< CopModule >
{
public :
	DECLARE_LIBID(LIBID_opLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OP, "{66b9c175-82f2-45e9-af86-58ad5ded5adc}")
};

extern class CopModule _AtlModule;
