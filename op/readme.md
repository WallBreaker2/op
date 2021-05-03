# 文件目录
* core: op插件各个模块依赖的公共数据结构，函数，宏和全局变量等
* background：前台截图，后台(gdi,opengl,dx）截图；鼠标键盘操作
* iamgeProc:找图，OCR等图像操作功能的实现
* algorithm：op项目实现的一些算法
* 3rdpart: 依赖的第三方库，注意此类库通常较小，因此将源码完全引用至op项目，对于较大的库，如blackbone，仅引用其头文件
* interface: op插件提供给用户的com接口，c++接口等
* test: op插件的实验性代码
* tool: op插件的一些工具代码
* examples: op插件使用的一些例子