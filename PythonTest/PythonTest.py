
from win32com.client import Dispatch

op=Dispatch("op.opsoft");

print("op ver:",op.Ver());

hwnd=op.FindWindow("Qt5QWindowIcon","QWidgetClassWindow");

if hwnd:
	print(hwnd);
	ret=op.BindWindow(hwnd,1,1,0,0);
	if ret:
		print("bind ok.");
		op.Sleep(1000);
		op.Capture("screen.bmp");
		x=0;y=0;
		
		r,x,y=op.FindPic(0,0,800,600,"test.bmp",0.9);
		print("x:",x,"y",y);
		if r==1:
			op.MoveTo(x,y);
			op.Sleep(1000);
			op.LeftClick();
		else:
			print("not find pic");
	else:
		print("bind false.");
else:
	print("invalid window.");
