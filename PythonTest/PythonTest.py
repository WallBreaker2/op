
from win32com.client import Dispatch
class Demo:
    def __init__(self):
        #创建com对象
        self.op=op=Dispatch("op.opsoft");
        self.hwnd=0;
        self.send_hwnd=0;
        print("init");
        

    def test_base(self):
         #输出插件版本号
         print("op ver:",self.op.Ver());
         print("path:",self.op.GetPath());
         self.op.SetShowErrorMsg(2);
         r=self.op.WinExec("notepad",1);
         print("Exec notepad:",r);



    def test_window_api(self):
        #测试窗口接口

        self.hwnd = self.op.FindWindow("","无标题 - 记事本");
        print("parent hwnd:",self.hwnd);
        if self.hwnd:
            self.send_hwnd=self.op.FindWindowEx(self.hwnd,"Edit","");
        print("child hwnd:",self.send_hwnd);
        return 0;

    def test_bkmode(self):
        r=self.op.BindWindow(self.hwnd,"gdi","normal","normal",0);
        if r == 0:
            print("bind false");
        return r;

    def test_bkmouse_bkkeypad(self):
        self.op.MoveTo(200,200);
        self.op.Sleep(200);
        self.op.LeftClick();
        self.op.Sleep(1000);
        r=self.op.SendString(self.send_hwnd,"Hello World!");
        print("SendString ret:",r);
        self.op.Sleep(1000);
        return 0;

    def test_bkimage(self):
        cr=self.op.GetColor(30,30);
        print("color of (30,30):",cr);
        ret=self.op.Capture(0,0,2000,2000,"screen.bmp");
        print("op.Capture ret:",ret);
        r,x,y=self.op.FindPic(0,0,100,100,"test.png","000000",1.0,0);
        print("op.FindPic:",r,x,y);
        return 0;

    def test_ocr(self):
        #ocr-设置字库
        r=self.op.SetDict(0,"test.dict");
        print("SetDict:",r);
        s=self.op.OcrAuto(0,0,100,100,1.0);
        print("ocr:",s);
        s=self.op.OcrEx(0,0,100,100,"000000-020202",1.0);
        print("OcrEx:",s);
        s=self.op.OcrAutoFromFile("src.png",1.0);
        print("OcrAutoFromFile:",s);
        return 0;
    def test_clear(self):
        self.op.UnBindWindow();


def test_all():
    demo=Demo();
    demo.test_base();
    demo.test_window_api();
    if demo.test_bkmode() == 0:
        return 0;
    demo.test_bkmouse_bkkeypad();
    demo.test_bkimage();
    demo.test_ocr();
    demo.test_clear();

    return 0;

#run all test
print("test begin");
test_all();
print("test end");


