// Copyright (c) Microsoft Corporation. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace Windows.UI.Composition
{
    //
    //  IMPORTANT:
    //  This interface needs to maintain parity with windows\published\main\DComp.w
    //
    [ComImport]
    [ComVisible(true)]
    [Guid("2056F1E3-7DC8-4D28-AD74-B817F3481BB9")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ICompositionCaptureTest
    {
        [PreserveSig]
        int RenderVisual(
            object visual,
            uint offsetX,
            uint offsetY,
            uint width,
            uint height,
            uint format,
            ref IntPtr hMap,
            ref IntPtr hEvent,
            out uint cbMap); // count in bytes of the map
    }
}

