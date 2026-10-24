cmake_minimum_required(VERSION 3.10)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OP_GTEST_STATIC_TRIPLET "x64-windows-static")
else()
  set(OP_GTEST_STATIC_TRIPLET "x86-windows-static")
endif()

set(OP_GTEST_STATIC_OK FALSE)
set(OP_GTEST_INCLUDE_DIR "")
set(OP_GTEST_LINK_LIBS "")
set(OP_VCPKG_INSTALLED_DIR "")

if(DEFINED VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${VCPKG_INSTALLED_DIR}")
elseif(DEFINED _VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${_VCPKG_INSTALLED_DIR}")
elseif(DEFINED ENV{VCPKG_ROOT})
  set(OP_VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
  get_filename_component(_op_toolchain_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
  get_filename_component(_op_vcpkg_scripts_dir "${_op_toolchain_dir}" DIRECTORY)
  get_filename_component(_op_vcpkg_root "${_op_vcpkg_scripts_dir}" DIRECTORY)
  set(OP_VCPKG_INSTALLED_DIR "${_op_vcpkg_root}/installed")
endif()

if(OP_VCPKG_INSTALLED_DIR)
  file(TO_CMAKE_PATH "${OP_VCPKG_INSTALLED_DIR}" OP_VCPKG_INSTALLED_DIR)
  set(OP_GTEST_STATIC_ROOT "${OP_VCPKG_INSTALLED_DIR}/${OP_GTEST_STATIC_TRIPLET}")
  set(OP_GTEST_STATIC_INCLUDE "${OP_GTEST_STATIC_ROOT}/include")
  set(OP_GTEST_STATIC_LIB_RELEASE "${OP_GTEST_STATIC_ROOT}/lib/gtest.lib")

  if(EXISTS "${OP_GTEST_STATIC_INCLUDE}/gtest/gtest.h"
     AND EXISTS "${OP_GTEST_STATIC_LIB_RELEASE}")
    set(OP_GTEST_INCLUDE_DIR "${OP_GTEST_STATIC_INCLUDE}")
    set(OP_GTEST_LINK_LIBS "${OP_GTEST_STATIC_LIB_RELEASE}")
    set(OP_GTEST_STATIC_OK TRUE)
    message(STATUS "Using static GoogleTest: ${OP_GTEST_STATIC_LIB_RELEASE}")
  endif()
endif()

if(NOT OP_GTEST_STATIC_OK)
  find_package(GTest CONFIG REQUIRED)
  set(OP_GTEST_LINK_LIBS GTest::gtest)
endif()

set(OP_HAS_OPENCV_SOURCES FALSE)
set(OP_OPENCV_INCLUDE_DIR "")
set(OP_OPENCV_LIBS "")
if(NOT DEFINED OPENCV_LIB_SUFFIX)
  set(OPENCV_LIB_SUFFIX "500")
endif()

function(_op_find_static_lib _op_out_var _op_lib_dir _op_lib_name)
  set(_op_candidates
    "${_op_lib_dir}/${_op_lib_name}.lib"
    "${_op_lib_dir}/lib${_op_lib_name}.a"
    "${_op_lib_dir}/${_op_lib_name}.a"
  )
  foreach(_op_candidate IN LISTS _op_candidates)
    if(EXISTS "${_op_candidate}")
      set(${_op_out_var} "${_op_candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${_op_out_var} "" PARENT_SCOPE)
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libop/opencv/TemplateMatcher.h"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libop/opencv/TemplateMatcher.cpp")
  set(_op_opencv_root "")
  if(DEFINED OPENCV_ROOT)
    set(_op_opencv_root "${OPENCV_ROOT}")
  elseif(DEFINED ENV{OPENCV_ROOT})
    set(_op_opencv_root "$ENV{OPENCV_ROOT}")
  endif()

  if(NOT _op_opencv_root STREQUAL "")
    file(TO_CMAKE_PATH "${_op_opencv_root}" _op_opencv_root)
    set(OP_OPENCV_INCLUDE_DIR "${_op_opencv_root}/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_op_opencv_arch_dir "x64")
    else()
      set(_op_opencv_arch_dir "x86")
    endif()
    # 工具集目录不能写死成 vc18：本机装出来的是 vc17（libop/CMakeLists.txt 早就按候选列表找了，
    # 这里只认 vc18 会导致"产品编进了 OpenCV、测试却没编进去"，27 条 OpenCV 用例被静默丢掉。
    set(_op_opencv_lib_dir_candidates
      "${_op_opencv_root}/${_op_opencv_arch_dir}/vc18/staticlib"
      "${_op_opencv_root}/${_op_opencv_arch_dir}/vc17/staticlib"
      "${_op_opencv_root}/${_op_opencv_arch_dir}/vc16/staticlib"
    )
    set(_op_opencv_lib_dir "")
    foreach(_op_candidate IN LISTS _op_opencv_lib_dir_candidates)
      if(EXISTS "${_op_candidate}")
        set(_op_opencv_lib_dir "${_op_candidate}")
        break()
      endif()
    endforeach()
    if(_op_opencv_lib_dir STREQUAL "")
      set(_op_opencv_lib_dir "${_op_opencv_root}/${_op_opencv_arch_dir}/vc18/staticlib")
    endif()

    set(_op_required_lib_names
      "opencv_calib${OPENCV_LIB_SUFFIX}"
      "opencv_objdetect${OPENCV_LIB_SUFFIX}"
      "opencv_stereo${OPENCV_LIB_SUFFIX}"
      "opencv_features${OPENCV_LIB_SUFFIX}"
      "opencv_imgcodecs${OPENCV_LIB_SUFFIX}"
      "opencv_imgproc${OPENCV_LIB_SUFFIX}"
      "opencv_geometry${OPENCV_LIB_SUFFIX}"
      "opencv_flann${OPENCV_LIB_SUFFIX}"
      "opencv_core${OPENCV_LIB_SUFFIX}"
      "libclapack"
      "libjpeg-turbo"
      "libpng"
      "zlib"
    )

    set(_op_expected_libs "")
    set(_op_all_libs_exist TRUE)
    foreach(_op_lib_name IN LISTS _op_required_lib_names)
      _op_find_static_lib(_op_lib_path "${_op_opencv_lib_dir}" "${_op_lib_name}")
      if(_op_lib_path STREQUAL "")
        set(_op_all_libs_exist FALSE)
      else()
        list(APPEND _op_expected_libs "${_op_lib_path}")
      endif()
    endforeach()

    if(EXISTS "${OP_OPENCV_INCLUDE_DIR}/opencv2/core.hpp" AND _op_all_libs_exist)
      set(OP_HAS_OPENCV_SOURCES TRUE)
      set(OP_OPENCV_LIBS ${_op_expected_libs})
      message(STATUS "OpenCV tests enabled via manual static libraries: ${_op_opencv_lib_dir}")
    else()
      message(WARNING "OpenCV source adapter exists but required OpenCV headers or libraries were not found. Skipping OpenCV tests.")
    endif()
  else()
    message(WARNING "OpenCV source adapter exists but OPENCV_ROOT is not set. Skipping OpenCV tests.")
  endif()
endif()

set(OP_TEST_SOURCES
  "main.cpp"
  "test_support.cpp"
  "handle_compat_test.cpp"
  "op_algorithm_windows_test.cpp"
  "mouse_keyboard_test.cpp"
  "api_surface_test.cpp"
  "api_doc_test.cpp"
  "image_color_test.cpp"
  "capture_backend_test.cpp"
  "com_automation_test.cpp"
  "wgc_test.cpp"
  "ocr_test.cpp"
  "ocr_contract_test.cpp"
  "ocr_parse_test.cpp"
  "yolo_parse_test.cpp"
  "hook_capture_e2e_test.cpp"
  "yolo_contract_test.cpp"
  "yolo_test.cpp"
  "utils_test.cpp"
  "../libop/window/WindowLayout.cpp"
  "../libop/input/mouse/CursorShape.cpp"
  "../libop/ocr/OcrResponseParser.cpp"
  "../libop/yolo/YoloResponseParser.cpp"
  "${OP_COM_I_C}"
)
set_source_files_properties("${OP_COM_I_C}" PROPERTIES GENERATED TRUE)

if(OP_HAS_OPENCV_SOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/opencv_test.cpp")
  list(APPEND OP_TEST_SOURCES
    "opencv_test.cpp"
    "../libop/opencv/OpenCvHelpers.cpp"
    "../libop/opencv/TemplateMatcher.cpp"
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/edit_control_keypad_test.cpp")
  list(APPEND OP_TEST_SOURCES "keypad/edit_control_keypad_test.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/keypad_message_test.cpp")
  list(APPEND OP_TEST_SOURCES "keypad/keypad_message_test.cpp")
endif()

add_executable(op_test)
target_sources(op_test PRIVATE ${OP_TEST_SOURCES})
target_compile_definitions(op_test PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
if(DEFINED OP_ARCH_DEFINITION AND NOT OP_ARCH_DEFINITION STREQUAL "")
  target_compile_definitions(op_test PRIVATE "${OP_ARCH_DEFINITION}")
endif()
if(enable_wgc)
  target_compile_definitions(op_test PRIVATE OP_ENABLE_WGC)
endif()

target_include_directories(op_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/libop"
  "${PROJECT_SOURCE_DIR}/libop/window"
  "${PROJECT_SOURCE_DIR}/libop/image"
  "${PROJECT_SOURCE_DIR}/libop/ocr"
  "${PROJECT_SOURCE_DIR}/libop/opencv"
  "${OP_COM_GENERATED_DIR}"
  "${BLACKBONE_INCLUDE_DIR}"
  "${BLACKBONE_INCLUDE_DIR}/3rd_party"
)
if(OP_GTEST_INCLUDE_DIR)
  target_include_directories(op_test PRIVATE "${OP_GTEST_INCLUDE_DIR}")
endif()

target_link_libraries(op_test PRIVATE ${op_com} ${op_c_api} ${OP_GTEST_LINK_LIBS} dinput8.lib dxguid.lib dwmapi.lib crypt32.lib)

if(OP_HAS_OPENCV_SOURCES)
  target_include_directories(op_test PRIVATE ${OP_OPENCV_INCLUDE_DIR})
  target_link_libraries(op_test PRIVATE ${OP_OPENCV_LIBS})
endif()

# DX 输入注入的用例不走主入口：它们会挂钩目标进程的输入通路、抢前台与输入队列，
# 同进程里跑过一堆之后偶发只收到一半滚轮事件（§4.4，两次复现都在这个位置）。
# 这些用例在下面 WIN32 块里逐条注册成独立 ctest 项，每条一个进程。
add_test(NAME op_test COMMAND op_test "--gtest_filter=-MouseKeyTest.DxMode*")

# 后台（hook 注入）截图的端到端验证需要**真实的 D3D11 目标进程**：注入只发生在别的进程里，
# 同进程自测覆盖不到"宿主侧 Present 钩子把帧写进命名共享段 → 本进程读出来"这条链路。
if(WIN32)
  add_executable(d3d11_host_app "d3d11_host_app.cpp")
  target_compile_definitions(d3d11_host_app PRIVATE _UNICODE UNICODE)
  target_link_libraries(d3d11_host_app PRIVATE d3d11.lib dxgi.lib user32.lib gdi32.lib)
  add_dependencies(op_test d3d11_host_app)

  # 其余四个后端的真实目标进程（d3d9 / d3d10 / d3d12 / opengl）：
  # 一个程序按 --api= 走不同初始化，省得为每个后端各写一份窗口/消息循环。
  add_executable(gfx_host_app "gfx_host_app.cpp")
  target_compile_definitions(gfx_host_app PRIVATE _UNICODE UNICODE)
  target_link_libraries(gfx_host_app PRIVATE
    d3d9.lib d3d10.lib d3d12.lib dxgi.lib dxguid.lib opengl32.lib user32.lib gdi32.lib)
  add_dependencies(op_test gfx_host_app)

  # opengl.es（钩子挂在 libEGL!eglSwapBuffers）的最小 EGL/GLES2 目标程序。
  # ANGLE 的 DLL 不在系统目录里，程序运行时按 --egl-dir 或自动探测夸克网盘自带的
  # libEGL.dll；探测不到就打 HOST_FAIL 退出，用例那边 GTEST_SKIP。
  add_executable(egl_host_app "egl_host_app.cpp")
  target_compile_definitions(egl_host_app PRIVATE _UNICODE UNICODE)
  target_link_libraries(egl_host_app PRIVATE user32.lib gdi32.lib)
  add_dependencies(op_test egl_host_app)

  # 用例按"可执行文件同目录"找目标程序和注入用的 op_*.dll：注入路径由产品的
  # RuntimeEnvironment::getBasePath() 拼出，它取的是 op_test.exe 所在目录，
  # 而平时 DLL 只在 PATH 里，所以这里显式放一份到 op_test 旁边。
  add_custom_command(TARGET op_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:d3d11_host_app> $<TARGET_FILE_DIR:op_test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:gfx_host_app> $<TARGET_FILE_DIR:op_test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:egl_host_app> $<TARGET_FILE_DIR:op_test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${op_com}> $<TARGET_FILE_DIR:op_test>
    COMMENT "Staging d3d11_host_app, gfx_host_app, egl_host_app and ${op_com} next to op_test")

  # 跨位宽 e2e（32 位宿主注入 64 位目标）只能在 x86 构建里跑：需要 x64 侧的
  # op_x64.dll 和 64 位目标程序。源目录按顺序找（安装产物 → x64 构建目录），
  # 一个都没有就只打日志，用例那边 GTEST_SKIP —— 纯 x86 环境不该因此构建失败。
  if(NOT CMAKE_CL_64)
    # 顺序有讲究：优先取 x64 构建目录里的当前产物，最后才回退到安装目录 bin/x64
    # （安装目录只有跑过 install 才会更新，容易拿到旧 DLL 让用例出现"改完没生效"的假象）。
    set(OP_X64_ARTIFACT_DIRS
      "${PROJECT_SOURCE_DIR}/build/vs2022-x64-Release/tests/Release"
      "${PROJECT_SOURCE_DIR}/build/vs2022-x64-Release/libop/Release"
      "${PROJECT_SOURCE_DIR}/build/vs2022-x64-Release/tools/Release"
      "${PROJECT_SOURCE_DIR}/bin/x64")
    add_custom_command(TARGET op_test POST_BUILD
      COMMAND ${CMAKE_COMMAND}
              "-DSRC_DIRS=${OP_X64_ARTIFACT_DIRS}"
              "-DDST_DIR=$<TARGET_FILE_DIR:op_test>"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/stage_x64_artifacts.cmake"
      COMMENT "Staging x64 artifacts (op_x64.dll, gfx_host_app_x64.exe, op_inject64.exe) for cross-bitness e2e")
  endif()
endif()

# Windows 10 上 WGC 在一个进程里只有第一次会话可靠：同进程第二次 BindWindow(normal.wgc)
# 会在激活 GraphicsCaptureItem 工厂时抛异常，并且此后永久失败；而每条 WGC 用例单独起进程
# 时 12/12 全绿（证据见 doc/defect_fixes.md §29）。
# 因此主入口用 OP_SKIP_WGC_TESTS 跳过 WGC，再把每条 WGC 用例注册成独立 ctest 项，
# 让 ctest 真正覆盖 WGC，而不是被环境限制拖成一片红。
if(WIN32)
  set(OP_TEST_ENV "PATH=$<TARGET_FILE_DIR:${op_com}>;$<TARGET_FILE_DIR:${op_c_api}>;$ENV{PATH}")
  # 主入口只跑"不依赖交互式桌面"的用例：WGC 单独拆成每进程一项，
  # 真实截屏（GDI/DXGI）单独一项，避免它们被静默跳过或被别人拖成偶发红。
  set_tests_properties(op_test PROPERTIES
    ENVIRONMENT "OP_SKIP_WGC_TESTS=1;OP_SKIP_CAPTURE_TESTS=1;OP_SKIP_OCR_CONTRACT_TESTS=1;OP_SKIP_HOOK_E2E_TESTS=1;${OP_TEST_ENV}")

  # 真实截屏用例逐条一项（原来整组一项）。§4.5 那次偶发红只留下 "Failed" 而没有
  # 失败文本，整组一项时无从判断是哪条；拆开后哪条红一眼可见，也避免 8 条在同进程里
  # 共用桌面截图状态互相拖累。
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/capture_backend_test.cpp" OP_CAPTURE_TEST_LINES
       REGEX "^TEST_F\\(CaptureBackendTest, [A-Za-z0-9_]+\\)")
  foreach(op_capture_line IN LISTS OP_CAPTURE_TEST_LINES)
    string(REGEX REPLACE "^TEST_F\\(CaptureBackendTest, ([A-Za-z0-9_]+)\\).*$" "\\1" op_capture_name "${op_capture_line}")
    if(op_capture_name MATCHES "^DISABLED_")
      continue()
    endif()
    add_test(NAME "capture_backend_${op_capture_name}" COMMAND op_test "--gtest_filter=CaptureBackendTest.${op_capture_name}")
    set_tests_properties("capture_backend_${op_capture_name}" PROPERTIES ENVIRONMENT "${OP_TEST_ENV}" TIMEOUT 120)
  endforeach()

  # 后台（hook）截图要起真实目标进程并注入 op_*.dll，独占一项：
  # 注入会改进程级 hook 状态和命名共享段，和别的用例混跑会互相污染。
  # 用例集：d3d11 ×2（d3d11_host_app.exe）+ d3d9/d3d10/d3d12/opengl ×2（gfx_host_app.exe），
  # 每条都要起进程 + 注入 + 采样 3 秒以上，所以超时给到 300s（实测约 90s）。
  # §72.48：以前写死 HookCaptureE2ETest.*:HookCaptureGfxE2ETest.*，漏了
  # HookCaptureMultiChainE2ETest（§72.22 多条交换链写同一共享段的回归用例），
  # 那条用例在任何 ctest 项里都执行不到，报表却照样绿。改用通配覆盖全部派生套件。
  add_test(NAME hook_e2e COMMAND op_test "--gtest_filter=HookCapture*E2ETest.*")
# 严格模式：只挑"拿到就能跑、拿不到就该红"的那几条，覆盖全部可能的跳过点
# （OP_SKIP_HOOK_E2E_TESTS 门禁 / ANGLE 缺失 / x64 产物缺失）。环境齐全时它们必须真跑通过，
# 缺环境时必须失败 —— 不能像 gtest 默认那样把跳过算成通过（§72.51 R-02/R-03）。
add_test(NAME hook_require_e2e COMMAND op_test
         "--gtest_filter=HookCaptureE2ETest.D3D11BackgroundCaptureReadsFreshFrames:HookCaptureGfxE2ETest.OpenGLES*:HookCaptureGfxE2ETest.X86HostToX64*")
set_tests_properties(hook_require_e2e PROPERTIES
                     ENVIRONMENT "OP_REQUIRE_HOOK_E2E=1;${OP_TEST_ENV}"
                     TIMEOUT 300)
  set_tests_properties(hook_e2e PROPERTIES ENVIRONMENT "${OP_TEST_ENV}" TIMEOUT 600)

  # OCR 客户端契约用例用本地假服务端，必须独占进程（和 OcrFixture 的全局状态打架）。
  add_test(NAME ocr_contract COMMAND op_test "--gtest_filter=OcrContractTest.*:YoloContractTest.*")
  set_tests_properties(ocr_contract PROPERTIES ENVIRONMENT "${OP_TEST_ENV}")

  # 跨语言冒烟：用托管语言（C#）经 DllImport 调 C API，走的是真编组那条路。
  # C++ 测试直接链接 DLL，撞不出「返回的指针什么时候失效」「__stdcall 参数顺序」
  # 这类只有跨语言通道才暴露的问题 —— §72.57 那个短字符串 SSO 缺陷就是这条抓到的。
  #
  # dotnet SDK 不是本仓库的构建依赖，所以：找不到就干脆不注册；注册了但 SDK/产物
  # 缺失时脚本退 77，由 SKIP_RETURN_CODE 认成 SKIP 而不是红。
  find_program(OP_DOTNET_EXE dotnet)
  find_program(OP_POWERSHELL_EXE powershell)
  if(OP_DOTNET_EXE AND OP_POWERSHELL_EXE)
    if(CMAKE_CL_64)
      set(OP_SMOKE_ARCH x64)
    else()
      set(OP_SMOKE_ARCH x86)
    endif()
    add_test(NAME csharp_smoke
             COMMAND "${OP_POWERSHELL_EXE}" -NoProfile -ExecutionPolicy Bypass
                     -File "${PROJECT_SOURCE_DIR}/scripts/run_csharp_smoke.ps1"
                     -Arch ${OP_SMOKE_ARCH})
    set_tests_properties(csharp_smoke PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
  else()
    message(STATUS "dotnet or powershell not found: cross-language smoke test not registered")
  endif()

  # 只有 enable_wgc=ON 时 wgc_test.cpp 才会被编译进来；OFF 构建下不要注册这些 ctest 项，
  # 否则 --gtest_filter 匹配不到用例，gtest 退出码为 1，会把合法配置判成红。
  if(enable_wgc)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/wgc_test.cpp" OP_WGC_TEST_LINES
       REGEX "^TEST_F\\(WgcTest, [A-Za-z0-9_]+\\)")
  foreach(op_wgc_line IN LISTS OP_WGC_TEST_LINES)
    string(REGEX REPLACE "^TEST_F\\(WgcTest, ([A-Za-z0-9_]+)\\).*$" "\\1" op_wgc_name "${op_wgc_line}")
    if(op_wgc_name MATCHES "^DISABLED_")
      continue()
    endif()
    add_test(NAME "wgc_${op_wgc_name}" COMMAND op_test "--gtest_filter=WgcTest.${op_wgc_name}")
    set_tests_properties("wgc_${op_wgc_name}" PROPERTIES ENVIRONMENT "${OP_TEST_ENV}")
  endforeach()
  endif()

  # DX 输入注入用例逐条独立进程（理由见 add_test(op_test) 上方的注释）。
  # 它们声明成 TEST(MouseKeyTest, DxMode*)，抽名字时按 TEST 匹配。
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/mouse_keyboard_test.cpp" OP_MOUSE_DX_TEST_LINES
       REGEX "^TEST\\(MouseKeyTest, DxMode[A-Za-z0-9_]+\\)")
  foreach(op_dx_line IN LISTS OP_MOUSE_DX_TEST_LINES)
    string(REGEX REPLACE "^TEST\\(MouseKeyTest, (DxMode[A-Za-z0-9_]+)\\).*$" "\\1" op_dx_name "${op_dx_line}")
    add_test(NAME "mouse_dx_${op_dx_name}" COMMAND op_test "--gtest_filter=MouseKeyTest.${op_dx_name}")
    set_tests_properties("mouse_dx_${op_dx_name}" PROPERTIES ENVIRONMENT "${OP_TEST_ENV}" TIMEOUT 120)
  endforeach()
else()
  set_tests_properties(op_test
    PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${op_com}>;$<TARGET_FILE_DIR:${op_c_api}>;$ENV{PATH}")
endif()

if(CMAKE_CL_64)
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  if(WIN32)
    install(TARGETS d3d11_host_app gfx_host_app egl_host_app DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  endif()
else()
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  if(WIN32)
    install(TARGETS d3d11_host_app gfx_host_app egl_host_app DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
    # bin/x86 里也放一份 x64 产物，这样在安装目录直接跑 op_test 时跨位宽用例也能跑。
    # 用 install(CODE ...) 而不是 install(FILES ...)：源文件可能根本不存在（纯 x86 环境），
    # install(FILES) 会在安装阶段直接报错。
    install(CODE "
      set(_x64_srcs \"${PROJECT_SOURCE_DIR}/bin/x64\")
      foreach(_pair \"op_x64.dll=op_x64.dll\" \"gfx_host_app_x64.exe=gfx_host_app.exe\" \"egl_host_app_x64.exe=egl_host_app.exe\")
        string(REPLACE \"=\" \";\" _parts \"\${_pair}\")
        list(GET _parts 0 _dst)
        list(GET _parts 1 _src)
        if(EXISTS \"\${_x64_srcs}/\${_src}\")
          file(COPY \"\${_x64_srcs}/\${_src}\" DESTINATION \"${PROJECT_SOURCE_DIR}/bin/x86\")
          if(NOT _src STREQUAL _dst)
            file(RENAME \"${PROJECT_SOURCE_DIR}/bin/x86/\${_src}\" \"${PROJECT_SOURCE_DIR}/bin/x86/\${_dst}\")
          endif()
        endif()
      endforeach()
    ")
  endif()
endif()

set(OP_DEBUG_SOURCES
  "debug_runner/main_test.cpp"
)
if(OP_HAS_OPENCV_SOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debug_runner/main_test.cpp")
  add_executable(op_debug)
  target_sources(op_debug PRIVATE ${OP_DEBUG_SOURCES})
  target_compile_definitions(op_debug PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
  target_include_directories(op_debug PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libop"
    "${PROJECT_SOURCE_DIR}/libop/window"
    "${PROJECT_SOURCE_DIR}/libop/image"
    "${PROJECT_SOURCE_DIR}/libop/ocr"
    "${PROJECT_SOURCE_DIR}/libop/opencv"
  )
  target_link_libraries(op_debug PRIVATE ${op_com})

  set_target_properties(op_debug PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:op_debug>"
    VS_DEBUGGER_COMMAND_ARGUMENTS "smoke"
  )

  if(CMAKE_CL_64)
    install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  else()
    install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  endif()
endif()
