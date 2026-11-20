using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Windows.Foundation;

namespace RenderBench;

// Helpers for the full-backend scenes S11-S23.
static unsafe partial class Win32
{
    // The window's client area as the screen shows it; when the screen copy is uniform (disconnected remote session),
    // the window's own content via PrintWindow.
    public static BgraImage CaptureClient(nint hwnd)
    {
        GetClientRect(hwnd, out RECT rect);
        int width = rect.Right - rect.Left, height = rect.Bottom - rect.Top;
        byte[] pixels = Capture(hwnd, width, height, usePrintWindow: false);
        if (IsUniform(pixels))
        {
            pixels = Capture(hwnd, width, height, usePrintWindow: true);
        }
        return new BgraImage(width, height, pixels);
    }

    // A screen rectangle (BitBlt from the screen DC), for the S15 presented-frame probe.
    public static byte[] CaptureScreen(int x, int y, int width, int height)
    {
        nint screen = GetDC(0);
        nint memory = CreateCompatibleDC(screen);
        nint bitmap = CreateCompatibleBitmap(screen, width, height);
        nint previous = SelectObject(memory, bitmap);
        BitBlt(memory, 0, 0, width, height, screen, x, y, 0x00CC0020 /* SRCCOPY */);
        SelectObject(memory, previous);
        var header = new BITMAPINFOHEADER { biSize = (uint)sizeof(BITMAPINFOHEADER), biWidth = width, biHeight = -height,
            biPlanes = 1, biBitCount = 32 };
        var pixels = new byte[width * height * 4];
        fixed (byte* data = pixels)
        {
            GetDIBits(memory, bitmap, 0, (uint)height, data, &header, 0);
        }
        DeleteObject(bitmap);
        DeleteDC(memory);
        ReleaseDC(0, screen);
        return pixels;
    }

    // Element bounds in client pixels.
    public static (int X, int Y, int Width, int Height) ElementClientRect(FrameworkElement element)
    {
        double scale = element.XamlRoot.RasterizationScale;
        var origin = element.TransformToVisual(null).TransformPoint(new Point(0, 0));
        return ((int)Math.Round(origin.X * scale), (int)Math.Round(origin.Y * scale),
            (int)Math.Round(element.ActualWidth * scale), (int)Math.Round(element.ActualHeight * scale));
    }

    public static (int X, int Y) ClientOrigin(nint hwnd)
    {
        var origin = new POINT();
        ClientToScreen(hwnd, ref origin);
        return (origin.X, origin.Y);
    }

    public sealed record MonitorInfo(int Left, int Top, int Right, int Bottom, uint Dpi);

    // Work areas and effective DPI of every monitor (S20 DPI change by moving the window to a monitor with another DPI).
    public static List<MonitorInfo> Monitors()
    {
        var list = new List<MonitorInfo>();
        var handle = GCHandle.Alloc(list);
        try
        {
            EnumDisplayMonitors(0, 0, &CollectMonitor, GCHandle.ToIntPtr(handle));
        }
        finally
        {
            handle.Free();
        }
        return list;
    }

    [UnmanagedCallersOnly]
    static int CollectMonitor(nint monitor, nint dc, RECT* rect, nint data)
    {
        var list = (List<MonitorInfo>)GCHandle.FromIntPtr(data).Target!;
        var info = new MONITORINFOEXW { cbSize = (uint)sizeof(MONITORINFOEXW) };
        GetMonitorInfoW(monitor, ref info);
        uint dpi = GetDpiForMonitor(monitor, 0 /* MDT_EFFECTIVE_DPI */, out uint x, out _) == 0 ? x : 96;
        list.Add(new MonitorInfo(info.rcWork.Left, info.rcWork.Top, info.rcWork.Right, info.rcWork.Bottom, dpi));
        return 1;
    }

    public static uint DpiForWindow(nint hwnd) => GetDpiForWindow(hwnd);

    [DllImport("user32.dll")] static extern bool EnumDisplayMonitors(nint dc, nint clip, delegate* unmanaged<nint, nint, RECT*, nint, int> callback, nint data);
    [DllImport("shcore.dll")] static extern int GetDpiForMonitor(nint monitor, int type, out uint dpiX, out uint dpiY);
    [DllImport("user32.dll")] static extern uint GetDpiForWindow(nint hwnd);
}

// S16: a D3D11 composition swap chain for a SwapChainPanel through raw COM vtable calls (no SharpDX/Vortice/Win2D).
// Each Present clears the back buffer to the given color.
sealed unsafe class D3DSwapChain : IDisposable
{
    nint _device, _context, _swapChain;
    static readonly Guid s_dxgiDevice = new("54ec77fa-1377-44e6-8c32-88fd5f44c84c");
    static readonly Guid s_dxgiFactory2 = new("50c83a1c-e072-4c48-87b0-3630fa36a6d0");
    static readonly Guid s_texture2D = new("6f15aaf2-d208-4e89-9ab4-489535d34f9c");
    static readonly Guid s_swapChainPanelNative = new("63aad0b8-7c24-40ff-85a8-640d944cc325");

    public D3DSwapChain(int width, int height)
    {
        nint device, context;
        // D3D_DRIVER_TYPE_HARDWARE, D3D11_CREATE_DEVICE_BGRA_SUPPORT; WARP when there is no hardware device.
        int hr = D3D11CreateDevice(0, 1, 0, 0x20, null, 0, 7, &device, null, &context);
        if (hr < 0)
        {
            Check(D3D11CreateDevice(0, 5 /* WARP */, 0, 0x20, null, 0, 7, &device, null, &context));
        }
        _device = device;
        _context = context;

        nint dxgiDevice = QueryInterface(_device, s_dxgiDevice);
        nint adapter, factory;
        Check(Call<nint>(dxgiDevice, 7, &adapter)); // IDXGIDevice::GetAdapter
        Guid factoryId = s_dxgiFactory2;
        Check(((delegate* unmanaged[Stdcall]<nint, Guid*, nint*, int>)VTable(adapter)[6])(adapter, &factoryId, &factory)); // GetParent
        var desc = new DXGI_SWAP_CHAIN_DESC1
        {
            Width = (uint)width, Height = (uint)height, Format = 87 /* B8G8R8A8_UNORM */, SampleCount = 1,
            BufferUsage = 0x20 /* RENDER_TARGET_OUTPUT */, BufferCount = 2, Scaling = 0 /* STRETCH */,
            SwapEffect = 3 /* FLIP_SEQUENTIAL */, AlphaMode = 1 /* PREMULTIPLIED */,
        };
        nint swapChain;
        // IDXGIFactory2::CreateSwapChainForComposition
        Check(((delegate* unmanaged[Stdcall]<nint, nint, DXGI_SWAP_CHAIN_DESC1*, nint, nint*, int>)VTable(factory)[24])(
            factory, _device, &desc, 0, &swapChain));
        _swapChain = swapChain;
        Marshal.Release(factory);
        Marshal.Release(adapter);
        Marshal.Release(dxgiDevice);
    }

    public void Attach(Microsoft.UI.Xaml.Controls.SwapChainPanel panel)
    {
        nint unknown = ((WinRT.IWinRTObject)panel).NativeObject.ThisPtr;
        nint native = QueryInterface(unknown, s_swapChainPanelNative);
        try
        {
            Check(((delegate* unmanaged[Stdcall]<nint, nint, int>)VTable(native)[3])(native, _swapChain)); // SetSwapChain
        }
        finally
        {
            Marshal.Release(native);
        }
    }

    public void Present(float r, float g, float b)
    {
        nint texture, view;
        Guid textureId = s_texture2D;
        Check(((delegate* unmanaged[Stdcall]<nint, uint, Guid*, nint*, int>)VTable(_swapChain)[9])(_swapChain, 0, &textureId, &texture)); // GetBuffer
        try
        {
            Check(((delegate* unmanaged[Stdcall]<nint, nint, void*, nint*, int>)VTable(_device)[9])(_device, texture, null, &view)); // CreateRenderTargetView
            float* color = stackalloc float[] { r, g, b, 1 };
            ((delegate* unmanaged[Stdcall]<nint, nint, float*, void>)VTable(_context)[50])(_context, view, color); // ClearRenderTargetView
            Marshal.Release(view);
        }
        finally
        {
            Marshal.Release(texture);
        }
        Check(((delegate* unmanaged[Stdcall]<nint, uint, uint, int>)VTable(_swapChain)[8])(_swapChain, 1, 0)); // Present
    }

    public void Dispose()
    {
        Release(ref _swapChain);
        Release(ref _context);
        Release(ref _device);
    }

    static void Release(ref nint obj)
    {
        if (obj != 0)
        {
            Marshal.Release(obj);
            obj = 0;
        }
    }

    static nint* VTable(nint obj) => *(nint**)obj;

    static int Call<T>(nint obj, int slot, T* result) where T : unmanaged
        => ((delegate* unmanaged[Stdcall]<nint, T*, int>)VTable(obj)[slot])(obj, result);

    static nint QueryInterface(nint obj, Guid iid)
    {
        Check(Marshal.QueryInterface(obj, in iid, out nint result));
        return result;
    }

    static void Check(int hr)
    {
        if (hr < 0)
        {
            Marshal.ThrowExceptionForHR(hr);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct DXGI_SWAP_CHAIN_DESC1
    {
        public uint Width, Height, Format;
        public int Stereo;
        public uint SampleCount, SampleQuality, BufferUsage, BufferCount, Scaling, SwapEffect, AlphaMode, Flags;
    }

    [DllImport("d3d11.dll")]
    static extern int D3D11CreateDevice(nint adapter, int driverType, nint software, uint flags, int* featureLevels,
        uint featureLevelCount, uint sdkVersion, nint* device, int* featureLevel, nint* context);
}
