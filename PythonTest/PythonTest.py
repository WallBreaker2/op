
from win32com.client import Dispatch

op=Dispatch("op.opsoft");

print("op ver:",op.Ver());
hwnd=op.FindWindow("","五子棋(Piskvork)");
r=op.AddDict(0,"dm_soft.txt");
print("AddDict:",r);
r=0;
if hwnd:
	r=op.BindWindow(hwnd,"gdi","normal","windows",0);
	if r:
		print("bind ok.");
		r=op.Sleep(1000);
		print("try screencap");
		r=op.capture("screen.bmp");
		s = op.Ocr(30,376,148,394,"2D2E2C-000000",1.0);
		print("ocr:",s);
		r,x,y=op.FindColor(0,0,600,800,"2D2E2C");
		print(r,x,y);
		op.MoveTo(x,y);
		print(op.GetColor(165,164));
		op.MoveTo(0,0);
	else:
		print("inject false.");
else:
    print("invalid window.");

op.UnBind();
print("test end");


