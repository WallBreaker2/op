#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define OP_CALL __stdcall
#if defined(OP_C_API_STATIC)
#define OP_C_API
#elif defined(OP_C_API_EXPORTS)
#define OP_C_API __declspec(dllexport)
#else
#define OP_C_API __declspec(dllimport)
#endif
#else
#define OP_CALL
#if defined(OP_C_API_STATIC)
#define OP_C_API
#elif defined(OP_C_API_EXPORTS)
#define OP_C_API __attribute__((visibility("default")))
#else
#define OP_C_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct op_c_context op_c_context;
typedef op_c_context *op_handle;

// Lifecycle
OP_C_API op_handle OP_CALL OpCreate(void);
OP_C_API void OP_CALL OpDestroy(op_handle handle);

// Basic
OP_C_API const wchar_t *OP_CALL OpRuntimeVer(void);
OP_C_API int OP_CALL OpRuntimeSetPath(op_handle handle, const wchar_t *path);
OP_C_API const wchar_t *OP_CALL OpRuntimeGetPath(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpRuntimeGetBasePath(op_handle handle);
OP_C_API int OP_CALL OpRuntimeGetID(op_handle handle);
OP_C_API int OP_CALL OpRuntimeGetLastError(op_handle handle);
OP_C_API int OP_CALL OpRuntimeSetShowErrorMsg(op_handle handle, int show_type);
OP_C_API int OP_CALL OpRuntimeSleep(op_handle handle, int millseconds);
OP_C_API int OP_CALL OpWindowInjectDll(op_handle handle, const wchar_t *process_name, const wchar_t *dll_name);
OP_C_API int OP_CALL OpImageEnablePicCache(op_handle handle, int enable);
OP_C_API int OP_CALL OpImageCapturePre(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpImageSetScreenDataMode(op_handle handle, int mode);

// Algorithm
OP_C_API const wchar_t *OP_CALL OpAlgorithmAStarFindPath(op_handle handle, int map_width, int map_height,
                                                const wchar_t *disable_points, int begin_x, int begin_y, int end_x,
                                                int end_y);
OP_C_API const wchar_t *OP_CALL OpAlgorithmFindNearestPos(op_handle handle, const wchar_t *all_pos, int type, int x, int y);

// Window and process
OP_C_API const wchar_t *OP_CALL OpWindowEnumWindow(op_handle handle, intptr_t parent, const wchar_t *title,
                                             const wchar_t *class_name, int filter);
OP_C_API const wchar_t *OP_CALL OpWindowEnumWindowByProcess(op_handle handle, const wchar_t *process_name,
                                                      const wchar_t *title, const wchar_t *class_name, int filter);
OP_C_API const wchar_t *OP_CALL OpWindowEnumProcess(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpWindowClientToScreen(op_handle handle, intptr_t hwnd, int *x, int *y);
OP_C_API intptr_t OP_CALL OpWindowFindWindow(op_handle handle, const wchar_t *class_name, const wchar_t *title);
OP_C_API intptr_t OP_CALL OpWindowFindWindowByProcess(op_handle handle, const wchar_t *process_name,
                                                const wchar_t *class_name, const wchar_t *title);
OP_C_API intptr_t OP_CALL OpWindowFindWindowByProcessId(op_handle handle, int process_id, const wchar_t *class_name,
                                                  const wchar_t *title);
OP_C_API intptr_t OP_CALL OpWindowFindWindowEx(op_handle handle, intptr_t parent, const wchar_t *class_name,
                                         const wchar_t *title);
OP_C_API int OP_CALL OpWindowGetClientRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2);
OP_C_API int OP_CALL OpWindowGetClientSize(op_handle handle, intptr_t hwnd, int *width, int *height);
OP_C_API intptr_t OP_CALL OpWindowGetForegroundFocus(op_handle handle);
OP_C_API intptr_t OP_CALL OpWindowGetForegroundWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpWindowGetMousePointWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpWindowGetPointWindow(op_handle handle, int x, int y);
OP_C_API const wchar_t *OP_CALL OpWindowGetProcessInfo(op_handle handle, int pid);
OP_C_API intptr_t OP_CALL OpWindowGetSpecialWindow(op_handle handle, int flag);
OP_C_API intptr_t OP_CALL OpWindowGetWindow(op_handle handle, intptr_t hwnd, int flag);
OP_C_API const wchar_t *OP_CALL OpWindowGetWindowClass(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpWindowGetWindowProcessId(op_handle handle, intptr_t hwnd);
OP_C_API const wchar_t *OP_CALL OpWindowGetWindowProcessPath(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpWindowGetWindowRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2);
OP_C_API int OP_CALL OpWindowGetWindowState(op_handle handle, intptr_t hwnd, int flag);
OP_C_API const wchar_t *OP_CALL OpWindowGetWindowTitle(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpWindowMoveWindow(op_handle handle, intptr_t hwnd, int x, int y);
OP_C_API int OP_CALL OpWindowScreenToClient(op_handle handle, intptr_t hwnd, int *x, int *y);
OP_C_API int OP_CALL OpWindowSendPaste(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpWindowSetClientSize(op_handle handle, intptr_t hwnd, int width, int height);
OP_C_API int OP_CALL OpWindowSetWindowState(op_handle handle, intptr_t hwnd, int flag);
OP_C_API int OP_CALL OpWindowSetWindowSize(op_handle handle, intptr_t hwnd, int width, int height);
OP_C_API int OP_CALL OpWindowLayoutWindows(op_handle handle, const wchar_t *hwnds, int layout_type, int columns,
                                     int start_x, int start_y, int gap_x, int gap_y, int size_mode,
                                     int window_width, int window_height, int anchor_mode);
OP_C_API int OP_CALL OpWindowSetWindowText(op_handle handle, intptr_t hwnd, const wchar_t *title);
OP_C_API int OP_CALL OpWindowSetWindowTransparent(op_handle handle, intptr_t hwnd, int trans);
OP_C_API int OP_CALL OpWindowSendString(op_handle handle, intptr_t hwnd, const wchar_t *str);
OP_C_API int OP_CALL OpWindowSendStringIme(op_handle handle, intptr_t hwnd, const wchar_t *str);
OP_C_API int OP_CALL OpWindowRunApp(op_handle handle, const wchar_t *cmdline, int mode, uint32_t *pid);
OP_C_API int OP_CALL OpWindowWinExec(op_handle handle, const wchar_t *cmdline, int cmdshow);
OP_C_API const wchar_t *OP_CALL OpWindowGetCmdStr(op_handle handle, const wchar_t *cmd, int millseconds);
OP_C_API int OP_CALL OpWindowSetClipboard(op_handle handle, const wchar_t *str);
OP_C_API const wchar_t *OP_CALL OpWindowGetClipboard(op_handle handle);
OP_C_API int OP_CALL OpRuntimeDelay(op_handle handle, int mis);
OP_C_API int OP_CALL OpRuntimeDelays(op_handle handle, int mis_min, int mis_max);

// Background binding
OP_C_API int OP_CALL OpBindingBindWindow(op_handle handle, intptr_t hwnd, const wchar_t *display, const wchar_t *mouse,
                                  const wchar_t *keypad, int mode);
OP_C_API int OP_CALL OpBindingBindWindowEx(op_handle handle, intptr_t display_hwnd, intptr_t input_hwnd,
                                    const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, int mode);
OP_C_API int OP_CALL OpUnBindWindow(op_handle handle);
OP_C_API int OP_CALL OpBindingUnbindWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpBindingGetBindWindow(op_handle handle);
OP_C_API int OP_CALL OpBindingIsBind(op_handle handle);

// Mouse
OP_C_API int OP_CALL OpMouseGetCursorPos(op_handle handle, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpMouseGetCursorShape(op_handle handle);
OP_C_API int OP_CALL OpMouseMoveR(op_handle handle, int x, int y);
OP_C_API int OP_CALL OpMouseMoveTo(op_handle handle, int x, int y);
OP_C_API const wchar_t *OP_CALL OpMouseMoveToEx(op_handle handle, int x, int y, int w, int h);
OP_C_API int OP_CALL OpMouseLeftClick(op_handle handle);
OP_C_API int OP_CALL OpMouseLeftDoubleClick(op_handle handle);
OP_C_API int OP_CALL OpMouseLeftDown(op_handle handle);
OP_C_API int OP_CALL OpMouseLeftUp(op_handle handle);
OP_C_API int OP_CALL OpMouseMiddleClick(op_handle handle);
OP_C_API int OP_CALL OpMouseMiddleDown(op_handle handle);
OP_C_API int OP_CALL OpMouseMiddleUp(op_handle handle);
OP_C_API int OP_CALL OpMouseRightClick(op_handle handle);
OP_C_API int OP_CALL OpMouseRightDown(op_handle handle);
OP_C_API int OP_CALL OpMouseRightUp(op_handle handle);
OP_C_API int OP_CALL OpMouseWheelDown(op_handle handle);
OP_C_API int OP_CALL OpMouseWheelUp(op_handle handle);
OP_C_API int OP_CALL OpMouseSetMouseDelay(op_handle handle, const wchar_t *type, int delay);

// Keyboard
OP_C_API int OP_CALL OpKeyboardGetKeyState(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyboardKeyDown(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyboardKeyDownChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpKeyboardKeyUp(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyboardKeyUpChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpKeyboardWaitKey(op_handle handle, int vk_code, int time_out);
OP_C_API int OP_CALL OpKeyboardKeyPress(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyboardKeyPressChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpKeyboardSetKeypadDelay(op_handle handle, const wchar_t *type, int delay);
OP_C_API int OP_CALL OpKeyboardKeyPressStr(op_handle handle, const wchar_t *key_str, int delay);

// Image and color
OP_C_API int OP_CALL OpImageCapture(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *file_name);
OP_C_API int OP_CALL OpImageCmpColor(op_handle handle, int x, int y, const wchar_t *color, double sim);
OP_C_API int OP_CALL OpImageFindColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                 double sim, int dir, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpImageFindColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                              const wchar_t *color, double sim, int dir);
OP_C_API int OP_CALL OpImageFindMultiColor(op_handle handle, int x1, int y1, int x2, int y2,
                                      const wchar_t *first_color, const wchar_t *offset_color, double sim, int dir,
                                      int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpImageFindMultiColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *first_color, const wchar_t *offset_color,
                                                   double sim, int dir);
OP_C_API int OP_CALL OpImageFindPic(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                               const wchar_t *delta_color, double sim, int dir, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpImageFindPicEx(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *files, const wchar_t *delta_color, double sim, int dir);
OP_C_API const wchar_t *OP_CALL OpImageFindPicExS(op_handle handle, int x1, int y1, int x2, int y2,
                                             const wchar_t *files, const wchar_t *delta_color, double sim, int dir);
OP_C_API int OP_CALL OpImageFindColorBlock(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                      double sim, int count, int height, int width, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpImageFindColorBlockEx(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *color, double sim, int count, int height,
                                                   int width);
OP_C_API const wchar_t *OP_CALL OpImageGetColor(op_handle handle, int x, int y);
OP_C_API int OP_CALL OpImageGetColorNum(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                   double sim);
OP_C_API int OP_CALL OpImageSetDisplayInput(op_handle handle, const wchar_t *mode);
OP_C_API int OP_CALL OpImageLoadPic(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpImageFreePic(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpImageLoadMemPic(op_handle handle, const wchar_t *file_name, void *data, int size);
OP_C_API int OP_CALL OpImageGetPicSize(op_handle handle, const wchar_t *pic_name, int *width, int *height);
OP_C_API uintptr_t OP_CALL OpImageGetScreenData(op_handle handle, int x1, int y1, int x2, int y2, int *ret);
OP_C_API uintptr_t OP_CALL OpImageGetScreenDataBmp(op_handle handle, int x1, int y1, int x2, int y2, int *size,
                                              int *ret);
OP_C_API void OP_CALL OpImageGetScreenFrameInfo(op_handle handle, int *frame_id, int *time);
OP_C_API const wchar_t *OP_CALL OpImageMatchPicName(op_handle handle, const wchar_t *pic_name);

// OpenCV
OP_C_API int OP_CALL OpOpenCvLoadTemplate(op_handle handle, const wchar_t *name, const wchar_t *file_path);
OP_C_API int OP_CALL OpOpenCvLoadMaskedTemplate(op_handle handle, const wchar_t *name, const wchar_t *template_path,
                                            const wchar_t *mask_path);
OP_C_API int OP_CALL OpOpenCvRemoveTemplate(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpOpenCvRemoveAllTemplates(op_handle handle);
OP_C_API int OP_CALL OpOpenCvHasTemplate(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpOpenCvGetTemplateCount(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpOpenCvGetAllTemplateNames(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpOpenCvGetOpenCvVersion(op_handle handle);
OP_C_API int OP_CALL OpOpenCvLoadTemplateList(op_handle handle, const wchar_t *template_list);
OP_C_API int OP_CALL OpOpenCvToGray(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvToBinary(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvToEdge(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvToOutline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvDenoise(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvEqualize(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvCLAHE(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                               double clip_limit, int tile_grid_size);
OP_C_API int OP_CALL OpOpenCvBlur(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                              const wchar_t *mode, int kernel_size);
OP_C_API int OP_CALL OpOpenCvSharpen(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                 double strength);
OP_C_API int OP_CALL OpOpenCvCropValid(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API const wchar_t *OP_CALL OpOpenCvConnectedComponents(op_handle handle, const wchar_t *src_file, double min_area);
OP_C_API const wchar_t *OP_CALL OpOpenCvFindContours(op_handle handle, const wchar_t *src_file, double min_area);
OP_C_API int OP_CALL OpOpenCvPreprocessPipeline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                            const wchar_t *pipeline);
OP_C_API int OP_CALL OpOpenCvCrop(op_handle handle, const wchar_t *src_file, int x, int y, int width, int height,
                              const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvResize(op_handle handle, const wchar_t *src_file, int width, int height,
                                const wchar_t *dst_file);
OP_C_API int OP_CALL OpOpenCvThreshold(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                   double threshold, double max_value, const wchar_t *mode);
OP_C_API int OP_CALL OpOpenCvInRange(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                 const wchar_t *color_space, const wchar_t *lower, const wchar_t *upper);
OP_C_API int OP_CALL OpOpenCvMorphology(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                    const wchar_t *mode, int kernel_size, int iterations);
OP_C_API int OP_CALL OpOpenCvThin(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                              const wchar_t *mode);
OP_C_API const wchar_t *OP_CALL OpOpenCvMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                  const wchar_t *template_name, double threshold, int dir,
                                                  int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpOpenCvMatchTemplateScale(op_handle handle, int x, int y, int width, int height,
                                                       const wchar_t *template_name, const wchar_t *scales,
                                                       double threshold, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpOpenCvMatchAnyTemplate(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_names, double threshold, int dir,
                                                     int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpOpenCvMatchAllTemplates(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_names, double threshold, int dir,
                                                     int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpOpenCvFeatureMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                        const wchar_t *template_name, double threshold);
OP_C_API const wchar_t *OP_CALL OpOpenCvEdgeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_name, double threshold);
OP_C_API const wchar_t *OP_CALL OpOpenCvShapeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                      const wchar_t *template_name, double threshold);

// OCR
OP_C_API int OP_CALL OpOcrSetEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                                    const wchar_t *argv);
OP_C_API int OP_CALL OpOcrSetDict(op_handle handle, int idx, const wchar_t *file_name);
OP_C_API const wchar_t *OP_CALL OpOcrGetDict(op_handle handle, int idx, int font_index);
OP_C_API int OP_CALL OpOcrSetMemDict(op_handle handle, int idx, const wchar_t *data, int size);
OP_C_API int OP_CALL OpOcrUseDict(op_handle handle, int idx);
OP_C_API int OP_CALL OpOcrAddDict(op_handle handle, int idx, const wchar_t *dict_info);
OP_C_API int OP_CALL OpOcrSaveDict(op_handle handle, int idx, const wchar_t *file_name);
OP_C_API int OP_CALL OpOcrClearDict(op_handle handle, int idx);
OP_C_API int OP_CALL OpOcrGetDictCount(op_handle handle, int idx);
OP_C_API int OP_CALL OpOcrGetNowDict(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpOcrFetchWord(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *color, const wchar_t *word);
OP_C_API const wchar_t *OP_CALL OpOcrGetWordsNoDict(op_handle handle, int x1, int y1, int x2, int y2,
                                                 const wchar_t *color);
OP_C_API int OP_CALL OpOcrGetWordResultCount(op_handle handle, const wchar_t *result);
OP_C_API int OP_CALL OpOcrGetWordResultPos(op_handle handle, const wchar_t *result, int index, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpOcrGetWordResultStr(op_handle handle, const wchar_t *result, int index);
OP_C_API const wchar_t *OP_CALL OpOcrRecognize(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                      double sim);
OP_C_API const wchar_t *OP_CALL OpOcrRecognizeEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                        double sim);
OP_C_API int OP_CALL OpOcrFindStr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs,
                               const wchar_t *color, double sim, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpOcrFindStrEx(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *strs, const wchar_t *color, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrRecognizeAuto(op_handle handle, int x1, int y1, int x2, int y2, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrRecognizeFromFile(op_handle handle, const wchar_t *file_name,
                                              const wchar_t *color_format, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrRecognizeAutoFromFile(op_handle handle, const wchar_t *file_name, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrFindLine(op_handle handle, int x1, int y1, int x2, int y2,
                                           const wchar_t *color, double sim);

// Memory
OP_C_API int OP_CALL OpMemoryWriteData(op_handle handle, intptr_t hwnd, const wchar_t *address, const wchar_t *data,
                                 int size);
OP_C_API const wchar_t *OP_CALL OpMemoryReadData(op_handle handle, intptr_t hwnd, const wchar_t *address, int size);
OP_C_API int OP_CALL OpMemoryReadInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t *value);
OP_C_API int OP_CALL OpMemoryWriteInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t value);
OP_C_API int OP_CALL OpMemoryReadFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float *value);
OP_C_API int OP_CALL OpMemoryWriteFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float value);
OP_C_API int OP_CALL OpMemoryReadDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double *value);
OP_C_API int OP_CALL OpMemoryWriteDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double value);
OP_C_API const wchar_t *OP_CALL OpMemoryReadString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type,
                                             int len);
OP_C_API int OP_CALL OpMemoryWriteString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type,
                                   const wchar_t *value);

#ifdef __cplusplus
}
#endif
