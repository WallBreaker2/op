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
OP_C_API const wchar_t *OP_CALL OpVer(void);
OP_C_API int OP_CALL OpSetPath(op_handle handle, const wchar_t *path);
OP_C_API const wchar_t *OP_CALL OpGetPath(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpGetBasePath(op_handle handle);
OP_C_API int OP_CALL OpGetID(op_handle handle);
OP_C_API int OP_CALL OpGetLastError(op_handle handle);
OP_C_API int OP_CALL OpSetShowErrorMsg(op_handle handle, int show_type);
OP_C_API int OP_CALL OpSleep(op_handle handle, int millseconds);
OP_C_API int OP_CALL OpInjectDll(op_handle handle, const wchar_t *process_name, const wchar_t *dll_name);
OP_C_API int OP_CALL OpEnablePicCache(op_handle handle, int enable);
OP_C_API int OP_CALL OpCapturePre(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpSetScreenDataMode(op_handle handle, int mode);

// Algorithm
OP_C_API const wchar_t *OP_CALL OpAStarFindPath(op_handle handle, int map_width, int map_height,
                                                const wchar_t *disable_points, int begin_x, int begin_y, int end_x,
                                                int end_y);
OP_C_API const wchar_t *OP_CALL OpFindNearestPos(op_handle handle, const wchar_t *all_pos, int type, int x, int y);

// Window and process
OP_C_API const wchar_t *OP_CALL OpEnumWindow(op_handle handle, intptr_t parent, const wchar_t *title,
                                             const wchar_t *class_name, int filter);
OP_C_API const wchar_t *OP_CALL OpEnumWindowByProcess(op_handle handle, const wchar_t *process_name,
                                                      const wchar_t *title, const wchar_t *class_name, int filter);
OP_C_API const wchar_t *OP_CALL OpEnumProcess(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpClientToScreen(op_handle handle, intptr_t hwnd, int *x, int *y);
OP_C_API intptr_t OP_CALL OpFindWindow(op_handle handle, const wchar_t *class_name, const wchar_t *title);
OP_C_API intptr_t OP_CALL OpFindWindowByProcess(op_handle handle, const wchar_t *process_name,
                                                const wchar_t *class_name, const wchar_t *title);
OP_C_API intptr_t OP_CALL OpFindWindowByProcessId(op_handle handle, int process_id, const wchar_t *class_name,
                                                  const wchar_t *title);
OP_C_API intptr_t OP_CALL OpFindWindowEx(op_handle handle, intptr_t parent, const wchar_t *class_name,
                                         const wchar_t *title);
OP_C_API int OP_CALL OpGetClientRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2);
OP_C_API int OP_CALL OpGetClientSize(op_handle handle, intptr_t hwnd, int *width, int *height);
OP_C_API intptr_t OP_CALL OpGetForegroundFocus(op_handle handle);
OP_C_API intptr_t OP_CALL OpGetForegroundWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpGetMousePointWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpGetPointWindow(op_handle handle, int x, int y);
OP_C_API const wchar_t *OP_CALL OpGetProcessInfo(op_handle handle, int pid);
OP_C_API intptr_t OP_CALL OpGetSpecialWindow(op_handle handle, int flag);
OP_C_API intptr_t OP_CALL OpGetWindow(op_handle handle, intptr_t hwnd, int flag);
OP_C_API const wchar_t *OP_CALL OpGetWindowClass(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpGetWindowProcessId(op_handle handle, intptr_t hwnd);
OP_C_API const wchar_t *OP_CALL OpGetWindowProcessPath(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpGetWindowRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2);
OP_C_API int OP_CALL OpGetWindowState(op_handle handle, intptr_t hwnd, int flag);
OP_C_API const wchar_t *OP_CALL OpGetWindowTitle(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpMoveWindow(op_handle handle, intptr_t hwnd, int x, int y);
OP_C_API int OP_CALL OpScreenToClient(op_handle handle, intptr_t hwnd, int *x, int *y);
OP_C_API int OP_CALL OpSendPaste(op_handle handle, intptr_t hwnd);
OP_C_API int OP_CALL OpSetClientSize(op_handle handle, intptr_t hwnd, int width, int height);
OP_C_API int OP_CALL OpSetWindowState(op_handle handle, intptr_t hwnd, int flag);
OP_C_API int OP_CALL OpSetWindowSize(op_handle handle, intptr_t hwnd, int width, int height);
OP_C_API int OP_CALL OpLayoutWindows(op_handle handle, const wchar_t *hwnds, int layout_type, int columns,
                                     int start_x, int start_y, int gap_x, int gap_y, int size_mode,
                                     int window_width, int window_height, int anchor_mode);
OP_C_API int OP_CALL OpSetWindowText(op_handle handle, intptr_t hwnd, const wchar_t *title);
OP_C_API int OP_CALL OpSetWindowTransparent(op_handle handle, intptr_t hwnd, int trans);
OP_C_API int OP_CALL OpSendString(op_handle handle, intptr_t hwnd, const wchar_t *str);
OP_C_API int OP_CALL OpSendStringIme(op_handle handle, intptr_t hwnd, const wchar_t *str);
OP_C_API int OP_CALL OpRunApp(op_handle handle, const wchar_t *cmdline, int mode, uint32_t *pid);
OP_C_API int OP_CALL OpWinExec(op_handle handle, const wchar_t *cmdline, int cmdshow);
OP_C_API const wchar_t *OP_CALL OpGetCmdStr(op_handle handle, const wchar_t *cmd, int millseconds);
OP_C_API int OP_CALL OpSetClipboard(op_handle handle, const wchar_t *str);
OP_C_API const wchar_t *OP_CALL OpGetClipboard(op_handle handle);
OP_C_API int OP_CALL OpDelay(op_handle handle, int mis);
OP_C_API int OP_CALL OpDelays(op_handle handle, int mis_min, int mis_max);

// Background binding
OP_C_API int OP_CALL OpBindWindow(op_handle handle, intptr_t hwnd, const wchar_t *display, const wchar_t *mouse,
                                  const wchar_t *keypad, int mode);
OP_C_API int OP_CALL OpBindWindowEx(op_handle handle, intptr_t display_hwnd, intptr_t input_hwnd,
                                    const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, int mode);
OP_C_API int OP_CALL OpUnBindWindow(op_handle handle);
OP_C_API intptr_t OP_CALL OpGetBindWindow(op_handle handle);
OP_C_API int OP_CALL OpIsBind(op_handle handle);

// Mouse
OP_C_API int OP_CALL OpGetCursorPos(op_handle handle, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpGetCursorShape(op_handle handle);
OP_C_API int OP_CALL OpMoveR(op_handle handle, int x, int y);
OP_C_API int OP_CALL OpMoveTo(op_handle handle, int x, int y);
OP_C_API const wchar_t *OP_CALL OpMoveToEx(op_handle handle, int x, int y, int w, int h);
OP_C_API int OP_CALL OpMoveToSmooth(op_handle handle, int x, int y, int duration);
OP_C_API const wchar_t *OP_CALL OpMoveToExSmooth(op_handle handle, int x, int y, int w, int h, int duration);
OP_C_API int OP_CALL OpMovePath(op_handle handle, const wchar_t *path, int duration);
OP_C_API int OP_CALL OpDragPath(op_handle handle, const wchar_t *path, int duration);
OP_C_API int OP_CALL OpSetMouseTrajectory(op_handle handle, int mode, int min_duration, int max_duration, int jitter,
                                          int start_delay, int end_delay);
OP_C_API int OP_CALL OpLeftClick(op_handle handle);
OP_C_API int OP_CALL OpLeftDoubleClick(op_handle handle);
OP_C_API int OP_CALL OpLeftDown(op_handle handle);
OP_C_API int OP_CALL OpLeftUp(op_handle handle);
OP_C_API int OP_CALL OpMiddleClick(op_handle handle);
OP_C_API int OP_CALL OpMiddleDoubleClick(op_handle handle);
OP_C_API int OP_CALL OpMiddleDown(op_handle handle);
OP_C_API int OP_CALL OpMiddleUp(op_handle handle);
OP_C_API int OP_CALL OpRightClick(op_handle handle);
OP_C_API int OP_CALL OpRightDoubleClick(op_handle handle);
OP_C_API int OP_CALL OpRightDown(op_handle handle);
OP_C_API int OP_CALL OpRightUp(op_handle handle);
OP_C_API int OP_CALL OpXButton1Click(op_handle handle);
OP_C_API int OP_CALL OpXButton1DoubleClick(op_handle handle);
OP_C_API int OP_CALL OpXButton1Down(op_handle handle);
OP_C_API int OP_CALL OpXButton1Up(op_handle handle);
OP_C_API int OP_CALL OpXButton2Click(op_handle handle);
OP_C_API int OP_CALL OpXButton2DoubleClick(op_handle handle);
OP_C_API int OP_CALL OpXButton2Down(op_handle handle);
OP_C_API int OP_CALL OpXButton2Up(op_handle handle);
OP_C_API int OP_CALL OpWheel(op_handle handle, int delta);
OP_C_API int OP_CALL OpHWheel(op_handle handle, int delta);
OP_C_API int OP_CALL OpWheelDown(op_handle handle);
OP_C_API int OP_CALL OpWheelUp(op_handle handle);
OP_C_API int OP_CALL OpSetMouseDelay(op_handle handle, const wchar_t *type, int delay);

// Keyboard
OP_C_API int OP_CALL OpGetKeyState(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyDown(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyDownChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpKeyUp(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyUpChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpWaitKey(op_handle handle, int vk_code, int time_out);
OP_C_API int OP_CALL OpKeyPress(op_handle handle, int vk_code);
OP_C_API int OP_CALL OpKeyPressChar(op_handle handle, const wchar_t *vk_code);
OP_C_API int OP_CALL OpSetKeypadDelay(op_handle handle, const wchar_t *type, int delay);
OP_C_API int OP_CALL OpKeyPressStr(op_handle handle, const wchar_t *key_str, int delay);

// Image and color
OP_C_API int OP_CALL OpCapture(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *file_name);
OP_C_API int OP_CALL OpCmpColor(op_handle handle, int x, int y, const wchar_t *color, double sim);
OP_C_API int OP_CALL OpFindColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                 double sim, int dir, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpFindColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                              const wchar_t *color, double sim, int dir);
OP_C_API int OP_CALL OpFindMultiColor(op_handle handle, int x1, int y1, int x2, int y2,
                                      const wchar_t *first_color, const wchar_t *offset_color, double sim, int dir,
                                      int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpFindMultiColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *first_color, const wchar_t *offset_color,
                                                   double sim, int dir);
OP_C_API int OP_CALL OpFindPic(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                               const wchar_t *delta_color, double sim, int dir, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpFindPicEx(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *files, const wchar_t *delta_color, double sim, int dir);
OP_C_API const wchar_t *OP_CALL OpFindPicExS(op_handle handle, int x1, int y1, int x2, int y2,
                                             const wchar_t *files, const wchar_t *delta_color, double sim, int dir);
OP_C_API int OP_CALL OpFindColorBlock(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                      double sim, int count, int height, int width, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpFindColorBlockEx(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *color, double sim, int count, int height,
                                                   int width);
OP_C_API const wchar_t *OP_CALL OpGetColor(op_handle handle, int x, int y);
OP_C_API int OP_CALL OpGetColorNum(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                   double sim);
OP_C_API int OP_CALL OpSetDisplayInput(op_handle handle, const wchar_t *mode);
OP_C_API int OP_CALL OpLoadPic(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpFreePic(op_handle handle, const wchar_t *file_name);
OP_C_API int OP_CALL OpLoadMemPic(op_handle handle, const wchar_t *file_name, void *data, int size);
OP_C_API int OP_CALL OpGetPicSize(op_handle handle, const wchar_t *pic_name, int *width, int *height);
OP_C_API uintptr_t OP_CALL OpGetScreenData(op_handle handle, int x1, int y1, int x2, int y2, int *ret);
OP_C_API uintptr_t OP_CALL OpGetScreenDataBmp(op_handle handle, int x1, int y1, int x2, int y2, int *size,
                                              int *ret);
OP_C_API void OP_CALL OpGetScreenFrameInfo(op_handle handle, int *frame_id, int *time);
OP_C_API const wchar_t *OP_CALL OpMatchPicName(op_handle handle, const wchar_t *pic_name);

// OpenCV
OP_C_API int OP_CALL OpCvLoadTemplate(op_handle handle, const wchar_t *name, const wchar_t *file_path);
OP_C_API int OP_CALL OpCvLoadMaskedTemplate(op_handle handle, const wchar_t *name, const wchar_t *template_path,
                                            const wchar_t *mask_path);
OP_C_API int OP_CALL OpCvRemoveTemplate(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpCvRemoveAllTemplates(op_handle handle);
OP_C_API int OP_CALL OpCvHasTemplate(op_handle handle, const wchar_t *name);
OP_C_API int OP_CALL OpCvGetTemplateCount(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpCvGetAllTemplateNames(op_handle handle);
OP_C_API const wchar_t *OP_CALL OpCvGetOpenCvVersion(op_handle handle);
OP_C_API int OP_CALL OpCvLoadTemplateList(op_handle handle, const wchar_t *template_list);
OP_C_API int OP_CALL OpCvToGray(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvToBinary(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvToEdge(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvToOutline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvDenoise(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvEqualize(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvCLAHE(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                               double clip_limit, int tile_grid_size);
OP_C_API int OP_CALL OpCvBlur(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                              const wchar_t *mode, int kernel_size);
OP_C_API int OP_CALL OpCvSharpen(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                 double strength);
OP_C_API int OP_CALL OpCvCropValid(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file);
OP_C_API const wchar_t *OP_CALL OpCvConnectedComponents(op_handle handle, const wchar_t *src_file, double min_area);
OP_C_API const wchar_t *OP_CALL OpCvFindContours(op_handle handle, const wchar_t *src_file, double min_area);
OP_C_API int OP_CALL OpCvPreprocessPipeline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                            const wchar_t *pipeline);
OP_C_API int OP_CALL OpCvCrop(op_handle handle, const wchar_t *src_file, int x, int y, int width, int height,
                              const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvResize(op_handle handle, const wchar_t *src_file, int width, int height,
                                const wchar_t *dst_file);
OP_C_API int OP_CALL OpCvThreshold(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                   double threshold, double max_value, const wchar_t *mode);
OP_C_API int OP_CALL OpCvInRange(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                 const wchar_t *color_space, const wchar_t *lower, const wchar_t *upper);
OP_C_API int OP_CALL OpCvMorphology(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                    const wchar_t *mode, int kernel_size, int iterations);
OP_C_API int OP_CALL OpCvThin(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                              const wchar_t *mode);
OP_C_API const wchar_t *OP_CALL OpCvMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                  const wchar_t *template_name, double threshold, int dir,
                                                  int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpCvMatchTemplateScale(op_handle handle, int x, int y, int width, int height,
                                                       const wchar_t *template_name, const wchar_t *scales,
                                                       double threshold, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpCvMatchAnyTemplate(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_names, double threshold, int dir,
                                                     int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpCvMatchAllTemplates(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_names, double threshold, int dir,
                                                     int strip_mode, int method, int color_mode);
OP_C_API const wchar_t *OP_CALL OpCvFeatureMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                        const wchar_t *template_name, double threshold);
OP_C_API const wchar_t *OP_CALL OpCvEdgeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                     const wchar_t *template_name, double threshold);
OP_C_API const wchar_t *OP_CALL OpCvShapeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                      const wchar_t *template_name, double threshold);

// OCR
OP_C_API int OP_CALL OpSetOcrEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                                    const wchar_t *argv);
OP_C_API int OP_CALL OpSetYoloEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                                     const wchar_t *argv);
OP_C_API const wchar_t *OP_CALL OpYoloDetect(op_handle handle, int x1, int y1, int x2, int y2, double conf,
                                             double iou);
OP_C_API const wchar_t *OP_CALL OpYoloDetectFromFile(op_handle handle, const wchar_t *file_name, double conf,
                                                     double iou);
OP_C_API int OP_CALL OpSetDict(op_handle handle, int idx, const wchar_t *file_name);
OP_C_API const wchar_t *OP_CALL OpGetDict(op_handle handle, int idx, int font_index);
OP_C_API int OP_CALL OpSetMemDict(op_handle handle, int idx, const void *data, int size);
OP_C_API int OP_CALL OpUseDict(op_handle handle, int idx);
OP_C_API int OP_CALL OpAddDict(op_handle handle, int idx, const wchar_t *dict_info);
OP_C_API int OP_CALL OpSaveDict(op_handle handle, int idx, const wchar_t *file_name);
OP_C_API int OP_CALL OpClearDict(op_handle handle, int idx);
OP_C_API int OP_CALL OpGetDictCount(op_handle handle, int idx);
OP_C_API int OP_CALL OpGetNowDict(op_handle handle);
OP_C_API int OP_CALL OpSetBinaryPreprocess(op_handle handle, int mode, int isolated_threshold,
                                           int min_component_area, int bridge_gap);
OP_C_API int OP_CALL OpGetBinaryPreprocess(op_handle handle, int *mode, int *isolated_threshold,
                                           int *min_component_area, int *bridge_gap);
OP_C_API const wchar_t *OP_CALL OpFetchWord(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *color, const wchar_t *word);
OP_C_API const wchar_t *OP_CALL OpFetchWordEx(op_handle handle, int x1, int y1, int x2, int y2,
                                              const wchar_t *color, double sim, const wchar_t *word);
OP_C_API const wchar_t *OP_CALL OpExtractWordRects(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *color, double sim, int min_word_h);
OP_C_API const wchar_t *OP_CALL OpExtractWordRectsEx(op_handle handle, int x1, int y1, int x2, int y2,
                                                     const wchar_t *color, double sim, int min_word_w,
                                                     int min_word_h, int padding);
OP_C_API const wchar_t *OP_CALL OpFetchWords(op_handle handle, int x1, int y1, int x2, int y2,
                                             const wchar_t *color, double sim, const wchar_t *words,
                                             int min_word_h);
OP_C_API const wchar_t *OP_CALL OpFetchWordsEx(op_handle handle, int x1, int y1, int x2, int y2,
                                               const wchar_t *color, double sim, const wchar_t *words,
                                               int min_word_w, int min_word_h, int padding);
OP_C_API const wchar_t *OP_CALL OpFetchWordsByRects(op_handle handle, int x1, int y1, int x2, int y2,
                                                    const wchar_t *color, double sim, const wchar_t *words,
                                                    const wchar_t *rects);
OP_C_API const wchar_t *OP_CALL OpGetBinaryPreview(op_handle handle, int x1, int y1, int x2, int y2,
                                                   const wchar_t *color, double sim, int *ret);
OP_C_API const wchar_t *OP_CALL OpGetWordPreview(op_handle handle, const wchar_t *dict_info, int *ret);
OP_C_API const wchar_t *OP_CALL OpCheckWordDict(op_handle handle, const wchar_t *dict_info, int *ret);
OP_C_API const wchar_t *OP_CALL OpNormalizeWordDict(op_handle handle, const wchar_t *dict_info, int *ret);
OP_C_API const wchar_t *OP_CALL OpRenameWordDict(op_handle handle, const wchar_t *dict_info,
                                                 const wchar_t *words, int *ret);
OP_C_API const wchar_t *OP_CALL OpGetWordsNoDict(op_handle handle, int x1, int y1, int x2, int y2,
                                                 const wchar_t *color);
OP_C_API int OP_CALL OpGetWordResultCount(op_handle handle, const wchar_t *result);
OP_C_API int OP_CALL OpGetWordResultPos(op_handle handle, const wchar_t *result, int index, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpGetWordResultStr(op_handle handle, const wchar_t *result, int index);
OP_C_API const wchar_t *OP_CALL OpOcr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                      double sim);
OP_C_API const wchar_t *OP_CALL OpOcrEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                        double sim);
OP_C_API int OP_CALL OpFindStr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs,
                               const wchar_t *color, double sim, int *x, int *y);
OP_C_API const wchar_t *OP_CALL OpFindStrEx(op_handle handle, int x1, int y1, int x2, int y2,
                                            const wchar_t *strs, const wchar_t *color, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrAuto(op_handle handle, int x1, int y1, int x2, int y2, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrFromFile(op_handle handle, const wchar_t *file_name,
                                              const wchar_t *color_format, double sim);
OP_C_API const wchar_t *OP_CALL OpOcrAutoFromFile(op_handle handle, const wchar_t *file_name, double sim);
OP_C_API const wchar_t *OP_CALL OpFindLine(op_handle handle, int x1, int y1, int x2, int y2,
                                           const wchar_t *color, double sim);

// Memory
OP_C_API int OP_CALL OpWriteData(op_handle handle, intptr_t hwnd, const wchar_t *address, const wchar_t *data,
                                 int size);
OP_C_API const wchar_t *OP_CALL OpReadData(op_handle handle, intptr_t hwnd, const wchar_t *address, int size);
OP_C_API int OP_CALL OpReadInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t *value);
OP_C_API int OP_CALL OpWriteInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t value);
OP_C_API int OP_CALL OpReadFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float *value);
OP_C_API int OP_CALL OpWriteFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float value);
OP_C_API int OP_CALL OpReadDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double *value);
OP_C_API int OP_CALL OpWriteDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double value);
OP_C_API const wchar_t *OP_CALL OpReadString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type,
                                             int len);
OP_C_API int OP_CALL OpWriteString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type,
                                   const wchar_t *value);

#ifdef __cplusplus
}
#endif
