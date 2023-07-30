Overview
===========
OP(operator & open)是一个开源插件(类似大漠插件).主要功能有:Windows消息模拟,后台截图，找图,字符识别(OCR)等。使用c++编写，源代码可编译为32/64位dll.op插件提供了两类接口:1）原生c++接口，可以让c/c++开发者方便调用；2）com接口，支持大多数编译型语言(c++,c#,vb,delphi等 以及脚本语言（python,lua等）的调用

OP插件是为了满足Windows平台下各种自动化操作和图像处理的需求而开发的一个轻量级、高效、易用的工具。它可以帮助开发者和用户实现各种复杂的任务，例如模拟键鼠操作、后台截图、图像识别、文字识别等。它适用于各种场景，例如办公自动化、软件测试、数据采集、图像处理等。
![ava](doc/class_struct.svg)
## 功能特色
- Windows消息模拟，支持常见的键盘消息和鼠标消息模拟。
- 支持常见的截图方式，gdi,dx（包括d3d9,d3d10,d3d11),opengl截图，支持常见模拟器（雷电，夜神）的最小化截图
- 找色找图,支持偏色，支持模糊识别
- 字符识别(OCR)
 >1. 传统识别算法：最大支持255 X 255 超大点阵，支持偏色，支持模糊识别，支持系统字库
 
 >2. 内部接入主流 ocr引擎（例如google的tesseract)，无需繁琐配置，一条命令即可完成识别
- 插件有32位和64位版本，支持32/64位程序调用

## Download
包含32位和64位插件，tool工具以及必要的第三方库等文件  
下载地址：[GitHub](https://github.com/WallBreaker2/op/releases)  

## 教程  
* [快速开始](https://github.com/WallBreaker2/op/wiki/quick_start)
* 函数说明[wiki](https://github.com/WallBreaker2/op/wiki)

## 编译
### 编译环境
* 操作系统: windows 10 64位
* 编译器: vs2022 MSVC 32/64
* 工具： cmake 3.24以上
* DirectX SDK: 最新的即可
### 第三方库
* [blackbone](https://github.com/DarthTon/Blackbone.git)(静态编译，链接方式MT)
编译完成后，设置环境变量BLACKBONE_ROOT为源码根目录(例如D:\workspace\Blackbone)
* [tesseract](https://github.com/tesseract-ocr/tesseract)(依赖库较多，推荐vcpkg编译)
* [kiero](https://github.com/Rebzzel/kiero.git)(已在源码内，无需安装)
* [minhook](https://github.com/TsudaKageyu/minhook.git)(已在源码内，无需安装)
* [QT5.12](https://download.qt.io/archive/qt/5.12/5.12.12/)(可选) 安装完成后设置环境变量QT_ROOT为Qt安装目录下的版本目录，例如D:\workspace\QT\5.12.12
* [Python32/64](https://www.python.org/downloads/)(可选) 安装完成后设置环境变量PYTHON32_ROOT为32位python安装目录，PYTHON64_ROOT为64位python安装目录
## 交流
* QQ group:743710486
* [Discussion](https://github.com/WallBreaker2/op/discussions)


## 参考
---
[1] [TSPLUG源码,TC company](https://github.com/tcplugins/tsplug)  
[2] [Kiero](https://github.com/Rebzzel/kiero.git)
