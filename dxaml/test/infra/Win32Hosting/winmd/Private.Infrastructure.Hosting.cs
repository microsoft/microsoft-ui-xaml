// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.Foundation.Metadata;

namespace Private.Infrastructure.Hosting
{
#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    [Windows.Foundation.Metadata.Guid(0xc89c134a, 0xbaae, 0x4dd5, 0xa3, 0xc4, 0xea, 0xb0, 0xcc, 0x39, 0x39, 0x8b)]
    public interface IWin32Host : IDisposable
    {
        Windows.Foundation.IAsyncOperation<object> GetDispatcherQueue();
        void SetWindowSizeOverride(Double width, Double height);

        // Need to implement tests that Close the current Window and open a new one in the same thread.
        // See test CanRecreateXamlSourceAfterNewWindow for details
        void Reset();

        ulong MainWindowHandle { get; }

        object Content
        {
            get;
            set;
        }

        void GCCollect();
        void DoEvents();
    }

    public delegate void ExceptionHandler(string message);

    // These values line up with the values of DPI_AWARENESS_CONTEXT handles.
    // See https://docs.microsoft.com/en-us/windows/desktop/hidpi/dpi-awareness-context
    //   Note that the ((DPI_AWARENESS_CONTEXT)-x) declarations are a cast to DPI_AWARENESS_CONTEXT, not a subtraction.
    public enum DpiAwarenessContext
    {
        Unaware = -1,
        SystemAware = -2,
        PerMonitorAware = -3,
        PerMonitorAwareV2 = -4,
        UnawareGdiScaled = -5,
    }

#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    [Windows.Foundation.Metadata.Guid(0x34a62d54, 0x177d, 0x4b1f, 0x84, 0xeb, 0x20, 0xf8, 0x9f, 0x2d, 0xb8, 0xec)]
    public interface IWin32HostFactory
    {
        void SetExceptionHandler(ExceptionHandler handler);

        Windows.Foundation.IAsyncOperation<object> Create(DpiAwarenessContext dpiAwarenessContext, bool initCore);
    }

}
