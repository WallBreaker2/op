# import moudles
from win32com.client import Dispatch
import sys
import time;
# create op instance
op = Dispatch("op.opsoft");

print(op.Ver())
print(op.GetBasePath())
print(op.MoveTo(3,30))
hwnd = op.FindWindow("SDL_app","M2007J3SC");

print(hwnd)
if hwnd:
	ret = op.BindWindow(hwnd,"normal","windows","windows",0);
	if ret == 1:
		print("bind ok");
		c = 0;
		
	else:
		print("bind false");
#op.RunApp("notepad",0);


