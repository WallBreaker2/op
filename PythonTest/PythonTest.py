
from win32com.client import Dispatch
op=Dispatch("op.opsoft");
print("op ver:",op.Ver());
hwnd=op.FindWindow("","op_test.txt - 记事本");
r=op.SetDict(0,"test.dict");
print("SetDict:",r);
r=0;
if hwnd:
	r=op.BindWindow(hwnd,"gdi","normal","windows",0);
	if r:
		print("bind ok.");
		r=op.Sleep(1000);
		print("try screencap");
		r=op.Capture(0,0,100,100,"screen.bmp");
		r,x,y=op.FindColor(0,0,121,159,"000000-050505",0.9,1);
		print(r,x,y);
		if r:
			op.MoveTo(x,y);
			op.LeftClick();
		print("ocr:",op.Ocr(0,0,100,100,"000000",1.0));
		print(op.GetColor(165,164));
		print("wait key 65");
		op.WaitKey(65,5000);
		op.UnBindWindow();
	else:
		print("bind false.");
else:
	print("invalid window.");

print("test end");


