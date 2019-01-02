
from win32com.client import Dispatch


print("hello world\n");

op=Dispatch("op.opsoft");
print("op ver:",op.Ver());

hwnd=op.FindWindow("","五子棋(Piskvork)");
print(hwnd);
if hwnd:
	ret=op.BindWindow(hwnd,1,0,0,0);
	if ret:
		print("bind ok.");
		op.Sleep(1000);
		op.Capture("screen.bmp");
		x=0;y=0;
		
		r,x,y=op.FindPic(0,0,800,600,"test.png",0.9);
		print("x:",x,"y",y);
		op.MoveTo(x,y);
		op.Sleep(1000);
		op.LeftClick();
	else:
		print("bind false.");
else:
	print("no window.");
