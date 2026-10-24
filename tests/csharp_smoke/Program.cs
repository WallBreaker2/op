using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpSmoke
{
    /// <summary>
    /// 从托管语言调用 op_c_api 的端到端冒烟。
    ///
    /// C++ 测试跑的是同一份 DLL 的 C++ 接口；这个程序走的是另一条路 ——
    /// DllImport + __stdcall + BSTR/wchar_t* 编组，能撞出 C++ 测试撞不到的问题
    /// （典型的是「返回的字符串指针什么时候失效」这类生命周期约定）。
    ///
    /// 刻意不用 MarshAs(UnmanagedType.LPWStr) 直接拿 string：
    /// 那样运行时会立刻拷贝，反而把「指针互相覆盖」的缺陷藏起来。
    /// 这里一律拿裸 IntPtr，自己决定什么时候读。
    /// </summary>
    internal static class Program
    {
#if OP_X86
        private const string Dll = "op_c_api_x86.dll";
#else
        private const string Dll = "op_c_api_x64.dll";
#endif

        private const CallingConvention Call = CallingConvention.StdCall;

        [DllImport(Dll, CallingConvention = Call)]
        private static extern IntPtr OpCreate();

        [DllImport(Dll, CallingConvention = Call)]
        private static extern void OpDestroy(IntPtr handle);

        [DllImport(Dll, CallingConvention = Call)]
        private static extern IntPtr OpVer();

        [DllImport(Dll, CallingConvention = Call)]
        private static extern int OpSetShowErrorMsg(IntPtr handle, int show_type);

        [DllImport(Dll, CallingConvention = Call, CharSet = CharSet.Unicode)]
        private static extern int OpBindWindow(IntPtr handle, IntPtr hwnd, string display,
                                               string mouse, string keypad, int mode);

        [DllImport(Dll, CallingConvention = Call)]
        private static extern int OpUnBindWindow(IntPtr handle);

        [DllImport(Dll, CallingConvention = Call, CharSet = CharSet.Unicode)]
        private static extern int OpCapture(IntPtr handle, int x1, int y1, int x2, int y2,
                                            string file_name);

        [DllImport(Dll, CallingConvention = Call)]
        private static extern IntPtr OpGetColor(IntPtr handle, int x, int y);

        private static int _failures;
        private static string _workDir;

        private static void Check(string name, bool ok, string detail)
        {
            Console.WriteLine("  [{0}] {1}: {2}", ok ? " OK " : "FAIL", name, detail);
            if (!ok)
            {
                ++_failures;
            }
        }

        private static string W(IntPtr pointer)
            => pointer == IntPtr.Zero ? null : Marshal.PtrToStringUni(pointer);

        private static string Abs(string name) => Path.Combine(_workDir, name);

        // 读 BMP 头拿真实宽高。比按 54 + w*h*4 反推字节数可靠得多 ——
        // 反推只能证明「字节数对得上」，证明不了图真的那么大。
        private static bool TryReadBmpSize(string path, out int width, out int height)
        {
            width = 0;
            height = 0;
            try
            {
                if (!File.Exists(path))
                {
                    return false;
                }
                byte[] header = new byte[30];
                using (FileStream stream = File.OpenRead(path))
                {
                    if (stream.Read(header, 0, header.Length) < header.Length)
                    {
                        return false;
                    }
                }
                if (header[0] != (byte)'B' || header[1] != (byte)'M')
                {
                    return false;
                }
                width = BitConverter.ToInt32(header, 18);
                height = BitConverter.ToInt32(header, 22);
                if (height < 0)
                {
                    height = -height;   // 顶朝下的位图用负高度表示
                }
                return width > 0 && height > 0;
            }
            catch (IOException)
            {
                return false;
            }
        }

        private static int Main()
        {
            Console.OutputEncoding = Encoding.UTF8;
            _workDir = Directory.GetCurrentDirectory();

            Console.WriteLine("工作目录: " + _workDir);
            Console.WriteLine("入口 DLL: " + Dll);

            IntPtr handle = OpCreate();
            if (handle == IntPtr.Zero)
            {
                Console.WriteLine("FAIL: OpCreate() 返回空句柄，插件没加载起来");
                return 1;
            }

            try
            {
                // 1) 版本号。能拿到就说明句柄创建和字符串返回通道都通了。
                string version = W(OpVer());
                Check("OpVer()", !string.IsNullOrEmpty(version), version ?? "<null>");

                // 2) 批处理脚本必须关掉弹框，否则出错时会被对话框卡住。
                Check("OpSetShowErrorMsg(0)", OpSetShowErrorMsg(handle, 0) == 1, "已关闭弹框");

                // 3) 绑桌面。
                int bindRet = OpBindWindow(handle, IntPtr.Zero, "normal", "normal", "normal", 0);
                Check("OpBindWindow(桌面, normal)", bindRet == 1, "ret=" + bindRet);

                if (bindRet != 1)
                {
                    Console.WriteLine("绑定失败，后续用例无法进行");
                }
                else
                {
                    // 4) 抓图存盘。
                    //    尺寸不靠猜参数语义，直接读 BMP 头拿真实宽高 ——
                    //    x2/y2 是「右下角开区间」还是「宽高」，x1=0 时分不出来（第 5 步）。
                    const int width = 320, height = 240;
                    string shot = Abs("csharp_smoke_shot.bmp");
                    if (File.Exists(shot))
                    {
                        File.Delete(shot);
                    }
                    int capRet = OpCapture(handle, 0, 0, width - 1, height - 1, shot);
                    int bw, bh;
                    bool gotSize = TryReadBmpSize(shot, out bw, out bh);
                    long size = File.Exists(shot) ? new FileInfo(shot).Length : 0;
                    Check("OpCapture 落盘",
                          capRet == 1 && gotSize,
                          "ret=" + capRet + "，" + size + " 字节，" + bw + "x" + bh);
                    Check("OpCapture(0,0,319,239) 得到 319x239",
                          bw == 319 && bh == 239,
                          bw + "x" + bh + "（" + (54 + (long)bw * bh * 4) + " 字节 = 54 + w*h*4）");

                    // 5) 非零起点，把 x2/y2 的语义定死：
                    //    49x49 说明 x2/y2 是右下角开区间（宽 = x2-x1）；
                    //    59x59 说明 x2/y2 就是宽高。
                    string shot2 = Abs("csharp_smoke_shot2.bmp");
                    if (File.Exists(shot2))
                    {
                        File.Delete(shot2);
                    }
                    int capRet2 = OpCapture(handle, 10, 10, 59, 59, shot2);
                    int bw2, bh2;
                    bool gotSize2 = TryReadBmpSize(shot2, out bw2, out bh2);
                    Check("OpCapture(10,10,59,59) 的宽高",
                          capRet2 == 1 && gotSize2 && bw2 == 49 && bh2 == 49,
                          bw2 + "x" + bh2 + "（49x49 = 右下角开区间；59x59 = 宽高）");

                    // 5) 取颜色，格式必须是 RRGGBB。
                    string color = W(OpGetColor(handle, 10, 10));
                    Check("OpGetColor(10,10)", color != null && color.Length == 6, color ?? "<null>");

                    // 6) 核心：返回值指针的生命周期。
                    //    §72.48 修过一个缺陷 —— call_string 用同一个静态缓冲区，
                    //    「返回值还没用就被下一次调用覆盖」。
                    //    这里拿裸指针：第一次的指针先不读，发起第二次调用，再回头读第一次。
                    //    如果实现回退到共用缓冲区，这里会读到第二个颜色而不是第一个。
                    IntPtr firstPointer = OpGetColor(handle, 10, 10);
                    string firstValue = W(firstPointer);
                    IntPtr secondPointer = OpGetColor(handle, 20, 20);
                    string secondValue = W(secondPointer);
                    string firstAfterSecond = W(firstPointer);

                    Check("两次调用的返回值各自独立（指针未被复用）",
                          firstValue != null && secondValue != null &&
                          firstValue.Length == 6 && secondValue.Length == 6 &&
                          firstValue == firstAfterSecond,
                          "第一次=" + firstValue + "，第二次=" + secondValue +
                          "，二次调用后重读第一次=" + firstAfterSecond);

                    // 7) 不可写路径必须返回 0，不能静默成功。
                    int badRet = OpCapture(handle, 0, 0, 15, 15, @"Z:\no_such_dir\should_fail.bmp");
                    Check("不可写路径返回失败", badRet == 0, "ret=" + badRet);

                    int unbindRet = OpUnBindWindow(handle);
                    Check("OpUnBindWindow()", unbindRet == 1, "ret=" + unbindRet);
                }
            }
            finally
            {
                OpDestroy(handle);
            }

            Console.WriteLine();
            if (_failures == 0)
            {
                Console.WriteLine("PASS: 全部通过");
                return 0;
            }
            Console.WriteLine("FAIL: " + _failures + " 项失败");
            return 1;
        }
    }
}
