
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


