## Introduction

After a long time, I find that kiero had too much responsibility for a library whose only real job is to tell you *where* a methods located. So I decided to create kiero v2, which focuses solely on the core purpose of the library: locating methods.


## Migrating from v1
```C++
// v1
if (kiero::init(kiero::RenderType::D3D9) != kiero::Status::Success) {
    return;
}

kiero::bind(42, (void**)&oEndScene, hkEndScene);
kiero::shutdown();
```

Same code on v2:

```C++
// v2
kiero::D3D9Output output;
auto error = kiero::locate<kiero::Implementation_D3D9>(nullptr, &output);
if (error != kiero::Error_Nil) {
    return;
}

auto oEndScene = output.device_methods[42];

// hook with your favorite hook library
MH_CreateHook(oEndScene, hkEndScene, nullptr);
MH_EnableHook(oEndScene);
```

### No more kiero::RenderType::Auto

Instead of trying to guess one of the graphical apis which application use, we suggest locating each of them:

```C++
struct {
  kiero::D3D9Output d3d9;
  kiero::D3D10Output d3d10;
  kiero::D3D11Output d3d11;
  kiero::D3D12Output d3d12;
  kiero::OpenGLOutput opengl;
  kiero::VulkanOutput vulkan;
  //kiero::MyOutput my;
} output;

if (GetModuleHandleA("d3d12.dll")) {
  kiero::locate<kiero::Implementation_D3D12>(nullptr, &output.d3d12);
}
if (GetModuleHandleA("d3d11.dll")) {
  kiero::locate<kiero::Implementation_D3D11>(nullptr, &output.d3d11);
}
if (GetModuleHandleA("d3d10.dll")) {
  kiero::locate<kiero::Implementation_D3D10>(nullptr, &output.d3d10);
}
if (GetModuleHandleA("d3d9.dll")) {
  kiero::locate<kiero::Implementation_D3D9>(nullptr, &output.d3d9);
}
if (GetModuleHandleA("opengl32.dll")) {
  kiero::locate<kiero::Implementation_OpenGL>(nullptr, &output.opengl);
}
if (GetModuleHandleA("vulkan-1.dll")) {
  kiero::locate<kiero::Implementation_Vulkan>(nullptr, &output.vulkan);
}
//if (GetModuleHandleA("supercoolapi.dll")) {
//  kiero::locate<kiero::Implementation_My>(nullptr, &output.my);
//}
```
