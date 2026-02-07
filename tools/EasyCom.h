

#define EASYCOM_API extern "C" __declspec(dllexport)

// EASYCOM_API int nEasyCom;

EASYCOM_API int setupA(const char *path);
EASYCOM_API int setupW(const wchar_t *path);
