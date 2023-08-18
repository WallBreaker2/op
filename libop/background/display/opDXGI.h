#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>
#include "IDisplay.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

//this code ref https://www.jianshu.com/p/e775b0f45376

class opDXGI:public IDisplay
{
public:
    opDXGI();
    ~opDXGI();
    //°ó¶¨
    long BindEx(HWND _hwnd, long render_type) override;
    //½â°ó
    long UnBindEx() override;

    virtual bool requestCapture(int x1, int y1, int w, int h, Image& img)override;

    bool InitD3D11Device();

    bool InitDuplication();

    bool GetDesktopFrame(ID3D11Texture2D** texture);


private:
    ID3D11Device* device_;
    ID3D11DeviceContext* deviceContext_;
    IDXGIOutputDuplication* duplication_;
    bool m_first;
    FrameInfo m_frameInfo;
    long dx_, dy_;
    D3D11_TEXTURE2D_DESC m_desc;
    void fmtFrameInfo(void* dst, HWND hwnd, int w, int h, bool inc=true);
};