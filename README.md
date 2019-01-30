OP
===========
OP(operator & open)是一个开源插件(类似大漠插件).主要特点:Windows消息模拟,应用程序截图，简单图像识别(S-IM),简单字符识别(S-OCR),以及其他实用功能...使用c++编写，提供高效稳定的算法实现.源代码可编译为32/64位dll.可为32位和64位应用程序调用,支持大多数语言的调用(c++,c#,vb,delphi,...)



***
## Overview
* [Windows消息模拟](#Windows消息模拟)
* [后台截图](#后台截图)
* [简单图像识别](#简单图像识别)
* [演示Demo](#演示Demo)
* [3rdpart-Lib](#3rdpart-Lib)
* [Reference](#Reference)

## [Download](https://github.com/WallBreaker2/op/releases)


#### Windows消息模拟
---
支持全局模式（normal)和Windows模式(windows)

#### 后台截图
---
前台,gdi后台,dx后台，opengl后台
#### 简单图像识别
---
图像定位,OCR（32x32点阵),支持多色，偏色，模糊识别
#### 演示Demo  
---
```Python

from win32com.client import Dispatch
op=Dispatch("op.opsoft");
print("op ver:",op.Ver());
hwnd=op.FindWindow("","新建文本文档.txt - 记事本");
r=op.SetDict(0,"dm_soft.txt");
print("SetDict:",r);
r=0;
if hwnd:
	r=op.BindWindow(hwnd,"gdi","normal","windows",0);
	if r:
		print("bind ok.");
		r=op.Sleep(1000);
		print("try screencap");
		r=op.capture("screen.bmp");
		s = op.Ocr(0,0,106,50,"000000-0f0f0f",1.0);
		print("ocr:",s);
		r,x,y=op.FindColor(0,0,121,159,"000000-050505");
		print(r,x,y);
		if r:
			op.MoveTo(x,y);
			op.LeftClick();
		print(op.GetColor(165,164));
		op.UnBind();
	else:
		print("bind false.");
else:
	print("invalid window.");

print("test end");



```
### 3rdpart-Lib  
---
[1].[c++ boost1.6](https://www.boost.org/)  
[2].[opencv3.4](https://opencv.org/)  
[3].[blackbone](https://github.com/DarthTon/Blackbone.git)  
[4].[minhook](https://github.com/TsudaKageyu/minhook.git)  

### Reference
---
[1] TSPLUG源码,TC company  
[2] [Kiero](https://github.com/Rebzzel/kiero.git)  
