## dinput鼠标和键盘消息获取简介
### dinput的普通流程
```c++
//1. 注册DirectInput并获取IDirectInput接口指针
DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION,IID_IDirectInput8, ( VOID** )&g_pDI, NULL )
//2. 获取系统鼠标
g_pDI->CreateDevice( GUID_SysMouse, &g_pMouse, NULL )
//3. 设置数据格式
g_pMouse->SetDataFormat( &g_dfMouse)
//4. 为该设备实例建立协作级别。合作级别决定了设备的这个实例如何与设备的其他实例和系统的其余部分交互。
/*
hwnd
Window handle to be associated with the device. This parameter must be a valid top-level window handle that belongs to the process. The window associated with the device must not be destroyed while it is still active in a DirectInput device.
dwFlags
Flags that describe the cooperative level associated with the device. The following flags are defined:
DISCL_BACKGROUND
该应用程序需要后台访问。如果授予后台访问权限，则可以随时获取设备，即使关联的窗口不是活动窗口。
DISCL_EXCLUSIVE
该应用程序需要独占访问。如果授予独占访问权限，则设备的其他实例在获取该设备时无法获得对该设备的独占访问权限。但是，始终允许对设备进行非独占访问，即使另一个应用程序已获得独占访问权限。以独占模式获取鼠标或键盘设备的应用程序在收到 WM_ENTERSIZEMOVE 和 WM_ENTERMENULOOP 消息时应始终取消获取设备.否则，用户无法操作菜单或移动和调整窗口大小。
DISCL_FOREGROUND
该应用程序需要前台访问。如果授予前台访问权限，则当关联的窗口移动到后台时，设备将自动取消获取。
DISCL_NONEXCLUSIVE
该应用程序需要非独占访问。对设备的访问不会干扰正在访问同一设备的其他应用程序
DISCL_NOWINKEY
禁用 Windows 徽标键。设置此标志可确保用户不会无意中退出应用程序。但请注意，当显示默认操作映射用户界面 (UI) 时，DISCL_NOWINKEY 不起作用，只要该 UI 存在，Windows 徽标键就会正常运行
*/
g_pMouse->SetCooperativeLevel( hDlg, DISCL_NONEXCLUSIVE |
                                                    DISCL_FOREGROUND ) )
//5.从 DirectInput 设备上的轮询对象中检索数据,如果设备不需要轮询，则调用此方法无效。如果不定期轮询需要轮询的设备，则不会从该设备接收新数据。调用此方法会导致 DirectInput 更新设备状态、生成输入事件（如果启用了缓冲数据）并设置通知事件（如果启用了通知）。
g_pMouse->Poll();

//6. 获得对输入设备的访问
 hr = g_pMouse->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = g_pMouse->Acquire();

//7. 从设备中检索即时数据。
g_pMouse->GetDeviceState( sizeof( MouseState ), &ms ) 
/*
cbData
Size of the buffer in the lpvData parameter, in bytes.
lpvData
Address of a structure that receives the current state of the device. The format of the data is established by a prior call to the IDirectInputDevice8::SetDataFormat method.
*/

```
The five predefined data formats require corresponding device state structures according to the following table:

| Data format |State structure|
|---|---|
c_dfDIMouse	|DIMOUSESTATE
c_dfDIMouse2	|DIMOUSESTATE2
c_dfDIKeyboard|	array of 256 bytes
c_dfDIJoystick|	DIJOYSTATE
c_dfDIJoystick2|	DIJOYSTATE2