# 把 x64 构建产物 stage 到 x86 测试目录，供"32 位宿主注入 64 位目标"的跨位宽 e2e 使用。
#
# 用法：
#   cmake -DSRC_DIRS="<候选源目录1>;<候选源目录2>" -DDST_DIR=<op_test.exe 所在目录>
#         -P tests/stage_x64_artifacts.cmake
#
# 需要的两个文件：
#   op_x64.dll          —— 注入到 64 位目标里的钩子 DLL（宿主侧按 getBasePath() 拼路径找它）
#   gfx_host_app.exe    —— 64 位目标进程，改名 gfx_host_app_x64.exe 避免和 32 位同名冲突
#   egl_host_app.exe    —— 64 位 EGL/GLES2 目标进程，改名 egl_host_app_x64.exe（32 位进程
#                          加载不了夸克网盘那份 64 位 libEGL.dll，opengl.es 必须走 64 位目标）
#   op_inject64.exe     —— 64 位注入助手，32 位宿主靠它往 64 位目标里装钩子
#
# 源目录按顺序找，第一个命中的就用；一个都没命中时只打 STATUS 不报错 ——
# 纯 x86 环境（没编过 x64）本来就拿不到，用例那边会 GTEST_SKIP，不该让构建失败。
if(NOT DEFINED DST_DIR OR NOT DEFINED SRC_DIRS)
  message(FATAL_ERROR "stage_x64_artifacts: DST_DIR / SRC_DIRS 都是必填参数")
endif()

# 目标文件名=源文件名
set(_wanted
  "op_x64.dll=op_x64.dll"
  "gfx_host_app_x64.exe=gfx_host_app.exe"
  "egl_host_app_x64.exe=egl_host_app.exe"
  "op_inject64.exe=op_inject64.exe")

foreach(_pair IN LISTS _wanted)
  string(REPLACE "=" ";" _parts "${_pair}")
  list(GET _parts 0 _dst_name)
  list(GET _parts 1 _src_name)

  set(_staged FALSE)
  foreach(_dir IN LISTS SRC_DIRS)
    if(EXISTS "${_dir}/${_src_name}")
      execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_dir}/${_src_name}" "${DST_DIR}/${_dst_name}"
        RESULT_VARIABLE _copy_result)
      if(_copy_result EQUAL 0)
        message(STATUS "stage_x64: ${_dir}/${_src_name} -> ${DST_DIR}/${_dst_name}")
        set(_staged TRUE)
        break()
      endif()
      message(WARNING "stage_x64: 拷贝 ${_dir}/${_src_name} 失败 result=${_copy_result}")
    endif()
  endforeach()

  if(NOT _staged)
    message(STATUS "stage_x64: 没找到 ${_src_name}（跨位宽用例会 GTEST_SKIP）")
  endif()
endforeach()
