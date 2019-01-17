
from win32com.client import Dispatch

op=Dispatch("op.opsoft");

print("op ver:",op.Ver());
hwnd=op.FindWindow("","五子棋(Piskvork)");
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
		s = op.Ocr(26,28,367,45,"2d2e2c-050505",1.0);
		print("ocr:",s);
		r,x,y=op.FindColor(113,151,121,159,"d1a244-050505");
		print(r,x,y);
		if r:
			op.MoveTo(x,y);
			op.LeftClick();
		print(op.GetColor(165,164));
	else:
		print("bind false.");
else:
    print("invalid window.");

op.UnBind();
print("test end");


