// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 EASYCOM_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// EASYCOM_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef EASYCOM_EXPORTS
#define EASYCOM_API extern "C"  __declspec(dllexport)
#else
#define EASYCOM_API extern "c"  __declspec(dllimport)
#endif


EASYCOM_API int nEasyCom;

EASYCOM_API int __stdcall setupA(const char* path);
EASYCOM_API int __stdcall setupW(const wchar_t* path);
