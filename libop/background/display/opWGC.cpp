
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <iostream>
#include <string>
#include "opWGC.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "./core/opEnv.h"
#include "./include/Image.hpp"

#ifdef _WIN32_WINNT_WIN11
opWGC::opWGC():device_(nullptr), item_(nullptr), framePool_(nullptr), session_(nullptr), d3dDevice_(nullptr), d3dDeviceContext_(nullptr), m_frameInfo()
{
}

opWGC::~opWGC()
{
	UnBindEx();
}


long opWGC::BindEx(HWND _hwnd, long render_type) {
	if (!Init(_hwnd))
	{
		setlog("Init wgc failed");
		return 0;
	}
	return 1;
}

long opWGC::UnBindEx() {
	if (framePool_) {
		try {
			framePool_.Close();
		}
		catch (winrt::hresult_error& err) {
			setlog("Direct3D11CaptureFramePool::Close (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
		}
		catch (...) {
			setlog("Direct3D11CaptureFramePool::Close (0x%08X)", winrt::to_hresult().value);
		}
	}

	
	if (session_) {
		try {
			session_.Close();
		}
		catch (winrt::hresult_error& err) {
			setlog("GraphicsCaptureSession::Close (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
		}
		catch (...) {
			setlog("GraphicsCaptureSession::Close (0x%08X)", winrt::to_hresult().value);
		}
	}
	
	if (device_) {
		try {
			device_.Close();
		}
		catch (winrt::hresult_error& err) {
			setlog("IDirect3DDevice::Close (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
		}
		catch (...) {
			setlog("IDirect3DDevice::Close (0x%08X)", winrt::to_hresult().value);
		}
	}
	
	if (d3dDevice_)
	{
		d3dDevice_->Release();
	}
	if (d3dDeviceContext_)
	{
		d3dDeviceContext_->Release();
	}

	session_ = nullptr;
	framePool_ = nullptr;
	device_ = nullptr;
	item_ = nullptr;
	d3dDeviceContext_ = nullptr;
	d3dDevice_ = nullptr;

	return 0;
}

bool opWGC::Init(HWND _hwnd)
{

	auto activation_factory = winrt::get_activation_factory<
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
	auto interop_factory =
		activation_factory.as<IGraphicsCaptureItemInterop>();
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};

	try {
		const HRESULT hr = interop_factory->CreateForWindow(
			_hwnd,
			winrt::guid_of<IGraphicsCaptureItem>(),
			reinterpret_cast<void**>(
				winrt::put_abi(item)));
		if (FAILED(hr))
			setlog("CreateForWindow (0x%08X)", hr);
	}
	catch (winrt::hresult_error& err) {
		setlog("CreateForWindow (0x%08X): %s",
			err.code().value,
			winrt::to_string(err.message()).c_str());
	}
	catch (...) {
		setlog("CreateForWindow (0x%08X)",
			winrt::to_hresult().value);
	}

	if (!item)
		return false;

	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
	D3D_FEATURE_LEVEL FeatureLevel;


	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			DriverTypes[DriverTypeIndex],
			nullptr, 0,
			FeatureLevels,
			NumFeatureLevels,
			D3D11_SDK_VERSION,
			&d3dDevice_,
			&FeatureLevel,
			&d3dDeviceContext_);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	d3dDevice_->GetImmediateContext(&d3dDeviceContext_);
	if (d3dDevice_ == nullptr || d3dDeviceContext_ == nullptr)
	{
		return false;
	}
	ComPtr<IDXGIDevice> dxgi_device;
	if (FAILED(d3dDevice_->QueryInterface(&dxgi_device))) {
		setlog("Failed to get DXGI device wgc");
		return false;
	}
		

	winrt::com_ptr<IInspectable> inspectable;
	if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.Get(),
		inspectable.put()))) {
		setlog("Failed to get WinRT device wgc");
		return false;
	
	}

	const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice
		device = inspectable.as<winrt::Windows::Graphics::DirectX::
		Direct3D11::IDirect3DDevice>();
	const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool
		frame_pool = winrt::Windows::Graphics::Capture::
		Direct3D11CaptureFramePool::Create(
			device,
			winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			2, 
			item.Size());
	const winrt::Windows::Graphics::Capture::GraphicsCaptureSession session =
		frame_pool.CreateCaptureSession(item);

	
	session.IsBorderRequired(false);

	
	session.IsCursorCaptureEnabled(false);
	

	item_ = item;
	device_ = device;
	framePool_ = frame_pool;
	session_ = session;

	
	try {
		session_.StartCapture();
	}
	catch (winrt::hresult_error& err) {
		setlog("StartCapture (0x%08X): %s", err.code().value,
			winrt::to_string(err.message()).c_str());
		return false;
	}
	catch (...) {
		setlog( "StartCapture (0x%08X)",
			winrt::to_hresult().value);
		return false;
	}
	//以下代码获取窗口大小，并从新初始化共享内存。  GetWindowRec方式获取的不准，缩放时
	Direct3D11CaptureFrame frame = {nullptr};
	//窗口最小化时，可能获取不到， 最开始的时候也获取不到， 尝试20次，如果尝试不成功就绑定失败
	int i = 20;
	while (!frame && i > 0)
	{
		frame = framePool_.TryGetNextFrame();
		i--;
	}
	if (!frame){
		return false;

	}

	winrt::com_ptr<ID3D11Texture2D> frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(
		frame.Surface());

	D3D11_TEXTURE2D_DESC desc;
	frame_surface->GetDesc(&desc);
	_width = desc.Width;
	_height = desc.Height;
	//从新初始化共享内存
	SAFE_DELETE(_shmem);
	SAFE_DELETE(_pmutex);

	int res_size = _width * _height * 4 + sizeof(FrameInfo);
	wsprintf(_shared_res_name, SHARED_RES_NAME_FORMAT, _hwnd);
	wsprintf(_mutex_name, MUTEX_NAME_FORMAT, _hwnd);
	//setlog(L"mem=%s mutex=%s", _shared_res_name, _mutex_name);
	//bind_release();
	try {
		_shmem = new sharedmem();
		_shmem->open_create(_shared_res_name, res_size);
		_pmutex = new promutex();
		_pmutex->open_create(_mutex_name);
	}
	catch (std::exception& e) {
		setlog("bkdisplay::re bind share mem %s exception:%s", _shared_res_name, e.what());
	}


	return true;
}



bool opWGC::requestCapture(int x1, int y1, int w, int h, Image& img) {
	img.create(w, h);
	ID3D11Texture2D* texture2D = nullptr;
	uint8_t* pDest = _shmem->data<byte>() + sizeof(FrameInfo);
	const Direct3D11CaptureFrame frame = framePool_.TryGetNextFrame();
	if (!frame) {
		return true;
	}

	winrt::com_ptr<ID3D11Texture2D> frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(
			frame.Surface());

	const winrt::Windows::Graphics::SizeInt32 frame_content_size =
		frame.ContentSize();

	
	
	D3D11_TEXTURE2D_DESC desc;
	frame_surface->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	d3dDevice_->CreateTexture2D(&desc, NULL, &texture2D);

	d3dDeviceContext_->CopyResource(texture2D, frame_surface.get());
	texture2D->GetDesc(&desc);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	d3dDeviceContext_->Map(texture2D, 0, D3D11_MAP_READ, 0, &mappedResource);

	uint8_t* pData = (uint8_t*)mappedResource.pData;
	_pmutex->lock();
	fmtFrameInfo(_shmem->data<byte>(), _hwnd, w, h);

	for (size_t i = 0; i < desc.Height; i++)
	{
		memcpy(pDest + i * desc.Width * 4, pData + i * mappedResource.RowPitch, desc.Width * 4);
	}
	_pmutex->unlock();
	texture2D->Release();
	texture2D = nullptr;

	
	if (x1 + w > desc.Width || y1 + + h > desc.Height) {
		setlog("error w and h src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", x1, w, desc.Width, y1, h, desc.Height);
		return false;
	}
	
	_pmutex->lock();
	uchar* pshare = _shmem->data<byte>();

	//将数据拷贝到目标
	for (int i = 0; i < h; i++) {
		memcpy(img.ptr<uchar>(i), pDest + (y1 + i) * 4 * desc.Width + x1 * 4, 4 * w);
	}
	_pmutex->unlock();
	return true;
}


void opWGC::fmtFrameInfo(void* dst, HWND hwnd, int w, int h,bool inc) {
	m_frameInfo.hwnd = (unsigned __int64)hwnd;
	m_frameInfo.frameId = inc ? m_frameInfo.frameId + 1 : m_frameInfo.frameId;
	m_frameInfo.time = ::GetTickCount64();
	m_frameInfo.width = w;
	m_frameInfo.height = h;
	m_frameInfo.fmtChk();
	memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}

#endif

