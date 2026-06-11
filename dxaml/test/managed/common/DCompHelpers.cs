// Copyright (c) Microsoft Corporation. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
using Microsoft.UI.Xaml;
using Microsoft.UI.Composition;
using WEX.Logging;
using WEX.Logging.Interop;
using WEX.TestExecution;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class VisualExtensions
    {
        private static Visual GetVisual(this UIElement element)
        {
            var visual = default(Visual);
            UIExecutor.Execute(() =>
            {
                visual = Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual(element);
                Verify.IsNotNull(visual, "visual");
                while (visual.Parent != null)
                {
                    visual = visual.Parent;
                }
            });
            return visual;
        }

        private static global::Windows.UI.Composition.ICompositionCaptureTest GetCompositionCaptureTest(this Visual visual)
        {
            var captureTest = default(global::Windows.UI.Composition.ICompositionCaptureTest);
            UIExecutor.Execute(() =>
            {
                var compositor = visual.Compositor;
                captureTest = ((object)compositor) as global::Windows.UI.Composition.ICompositionCaptureTest;
                Verify.IsNotNull(captureTest, "captureTest");
            });
            return captureTest;
        }

        public static async Task CaptureWindowAsync(this Window window, string id)
        {
            var visual = default(Visual);
            uint height = 0;
            uint width = 0;
            UIExecutor.Execute(() =>
            {
                height = (uint)window.Bounds.Height;
                width = (uint)window.Bounds.Width;
                var element = window.Content as UIElement;
                Verify.IsNotNull(element, "Window content is null, unable to capture window screenshot");
                visual = element.GetVisual();
                Verify.IsNotNull(visual, "Unable to get DComp visual for the Window Content, no screenshot can be captured");
                while(visual.Parent != null)
                {
                    visual = visual.Parent;
                }
            });
            using(var bitmap = await visual.CaptureVisualAsync(width, height))
            {
                await bitmap.LogAsync(id);
            }
        }

        public static async Task<SoftwareBitmap> CaptureVisualAsync(this UIElement element, uint captureWidth = 0, uint captureHeight = 0)
        {
            uint width = captureWidth;
            uint height = captureHeight;
            UIExecutor.Execute(() =>
            {
                var frameworkElement = element as FrameworkElement;
                if (width == 0)
                {
                    if (frameworkElement!=null)
                    {
                        width = (uint)frameworkElement.ActualWidth;
                    }
                }
                if (height == 0)
                {
                    if (frameworkElement!=null)
                    {
                        height = (uint)frameworkElement.ActualHeight;
                    }
                }
            });

            var visual = element.GetVisual();
            Verify.IsNotNull(visual, "Unable to get DComp visual for the UIElement, element cannot be captured");
            return await visual.CaptureVisualAsync(width, height);
        }

        public static async Task<SoftwareBitmap> CaptureVisualAsync(this Visual visual, uint captureWidth = 0, uint captureHeight = 0)
        {
            var captureTest = visual.GetCompositionCaptureTest();
            uint width = captureWidth;
            uint height = captureHeight;

            if (width == 0 || height == 0)
            {
                throw new NotImplementedException("Query height and width from Visual");
            }

            //   Render Visual:
            //   * This function is basically async and returns immediately after putting
            //     a MILCMD onto the batch for the application channel, and marks the device dirty.
            //   * It returns two handles by reference.
            //   * The first handle (hMap) is to a map of bits.
            //   * The second handle (hEvent) is to an event.
            //   * The event is signaled when a commit has happened
            //      and the after actual renderpass has been rendered and presented.
            //   * Once signaled, the bits are ready for us to grab.
            //   * Any changes to the tree before the implicit commit sends the batch to the
            //      Compositor will be reflected in the capture, even if made after the initial
            //      RenderVisual function call.
            //   * Any changes to the tree after the implicit commit may safely modify the tree,
            //      as they will be processed in a separate batch
            IntPtr hMap = IntPtr.Zero;
            IntPtr hEvent = IntPtr.Zero;
            uint cbMap;
            int hr = captureTest.RenderVisual(
                    visual,
                    0, // offset X
                    0, // offset y
                    width,
                    height,
                    (uint)BitmapPixelFormat.Bgra8,
                    ref hMap,
                    ref hEvent,
                    out cbMap);

            if (hr != 0)
            {
                throw new Exception("Render Visual Failed, ErrCode: " + hr);
            }

            // Block waiting for the bits in a worker thread and return them as a SoftwareBitmap
            var bitmap = new SoftwareBitmap(BitmapPixelFormat.Bgra8, (int)width, (int)height);
            var buffer = await Task.Run(() => GetVisualPixelBuffer(hEvent, hMap, cbMap));
            bitmap.CopyFromBuffer(buffer);

            return bitmap;
        }

        private static class Win32Interop
        {
            [DllImport(SYNC_API_DLL, SetLastError = true)]
            public static extern UInt32 WaitForSingleObject(IntPtr hHandle, UInt32 dwMilliseconds);

            [DllImport(FILE_API_DLL, SetLastError = true)]
            public static extern IntPtr MapViewOfFile(
                IntPtr hFileMappingObject,
                uint dwDesiredAccess,
                uint dwFileOffsetHigh,
                uint dwFileOffsetLow,
                IntPtr dwNumberOfBytesToMap);

            [DllImport(HANDLE_API_DLL, SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool CloseHandle(IntPtr hObject);

            [Flags]
            public enum FileMapAccess : uint
            {
                FileMapCopy = 0x0001,
                FileMapWrite = 0x0002,
                FileMapRead = 0x0004,
                FileMapAllAccess = 0x001f,
                FileMapExecute = 0x0020,
            }

            private const string FILE_API_DLL = "api-ms-win-core-file-l1-1-1.dll";
            private const string HANDLE_API_DLL = "api-ms-win-core-handle-l1-1-0.dll";
            private const string SYNC_API_DLL = "api-ms-win-core-synch-l1-1-1.dll";
        }

        private static IBuffer GetVisualPixelBuffer(IntPtr hEvent, IntPtr hMap, uint cbMap)
        {
            try
            {
                const int WAIT_OBJECT_0  = 0;
                const int WAIT_TIMEOUT   = 0x00000102;

                uint waitRet = Win32Interop.WaitForSingleObject(hEvent, (uint)EventTesterConfig.Timeout.TotalMilliseconds);
                if (waitRet == WAIT_OBJECT_0)
                {
                    IntPtr pbMap = Win32Interop.MapViewOfFile(hMap, (uint)Win32Interop.FileMapAccess.FileMapRead, 0, 0, IntPtr.Zero);

                    if (pbMap != IntPtr.Zero)
                    {
                        byte[] managedBuffer = new byte[cbMap];
                        Marshal.Copy(pbMap, managedBuffer, 0, (int)cbMap);
                        return managedBuffer.AsBuffer(); // Note that the finally block still executes
                    }
                    else
                    {
                        return null;
                    }
                }
                else if (waitRet == WAIT_TIMEOUT )
                {
                    throw new TimeoutException("RenderVisual Timed Out!");
                }
                else
                {
                    throw new InvalidOperationException($"RenderVisual event wait failed (0x{waitRet:x8}): {Marshal.GetLastWin32Error()}");
                }
            }
            finally
            {
                Win32Interop.CloseHandle(hMap);
                Win32Interop.CloseHandle(hEvent);
            }
        }
    }
}

