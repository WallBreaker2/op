
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
else:
	print("inject false.");
op.UnBind();
print("test end");


