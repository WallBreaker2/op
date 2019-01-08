
from win32com.client import Dispatch

op=Dispatch("op.opsoft");

print("op ver:",op.Ver());
hwnd=op.FindWindow("","HDRLighting");
r=0;
if hwnd:
	r=op.BindWindow(hwnd,"dx","windows","windows",0);
if r:
	print("bind ok.");
	r=op.Sleep(2000);
	print("try screencap");
	r=op.capture("dx_screen.bmp");
	r,x,y=op.FindPic(0,0,800,600,"test.bmp",0.9);
	print(r,x,y);
	if r:
		op.MoveTo(x,y);
else:
	print("inject false.");
op.UnBind();
print("test end");


