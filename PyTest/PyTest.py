
from win32com.client import Dispatch

from time import clock
op=Dispatch("op.opsoft");
dm=Dispatch("dm.dmsoft");
print(op.Ver());
print(dm.Ver());
dm.Reg("127.0.0.1","");
ret=op.WriteData(0,"<dm.dll>+1063D0","1",4);
print("write ret:",ret);

print(dm.MoveTo(30,30));
dm.EnableDisplayDebug(1);
ret=op.SetPath("C:\\Users\\wall\\Desktop");
print(ret);
ret=op.SetDict(0,"xx.txt");
print(ret);
t1=clock();
str=op.Ocr(0,0,2000,2000,"000000",1.0);
t2=clock();
print("op ocr time:",t2-t1);
print("ocr:",str);
# dm
dm.SetPath("C:\\Users\\wall\\Desktop");
print("dm setdict:",dm.SetDict(0,"xx.txt"));
dm.UseDict(0);
print(dm.GetColor(0,0));
t1=clock();
str=dm.Ocr(0,0,2000,2000,"000000",1.0);
t2=clock();
print("dm ocr time:",t2-t1);
print("ocr:",str);
ret=dm.CapturePre("dm_pre.bmp");
print("pre:",ret);

