// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;

namespace MUXControlsTestApp.Utilities
{
    internal static class WindowsRuntimeTestHelpers
    {
        private static readonly Guid IID_IInspectable = new Guid("AF86E2E0-B12D-4C6A-9C5A-D7AA65101E90");

        public static unsafe string GetRuntimeClassName(object instance)
        {
#if CSWINRT3_0
            if (!global::WindowsRuntime.InteropServices.WindowsRuntimeMarshal.TryGetNativeObject(instance, out void* nativeObject))
            {
                throw new ArgumentException("The object does not wrap a native Windows Runtime object.", nameof(instance));
            }
            IntPtr unknown = (IntPtr)nativeObject;
#else
            var reference = ((global::WinRT.IWinRTObject)instance).NativeObject;
            IntPtr unknown = reference.ThisPtr;
            Marshal.AddRef(unknown);
            GC.KeepAlive(reference);
#endif
            try
            {
                return GetRuntimeClassName(unknown);
            }
            finally
            {
                Marshal.Release(unknown);
            }
        }

        public static string GetActivationFactoryRuntimeClassName(string className)
        {
            IntPtr factory = GetActivationFactory(className);
            try
            {
                return GetRuntimeClassName(factory);
            }
            finally
            {
                Marshal.Release(factory);
            }
        }

        public static unsafe string ActivateInstance(string className)
        {
            IntPtr factory = GetActivationFactory(className);
            IntPtr instance = IntPtr.Zero;
            try
            {
                // IActivationFactory::ActivateInstance follows the six IInspectable slots.
                var activate = (delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, int>)(*(void***)factory)[6];
                Marshal.ThrowExceptionForHR(activate(factory, &instance));
                return instance == IntPtr.Zero ? null : GetRuntimeClassName(instance);
            }
            finally
            {
                if (instance != IntPtr.Zero)
                {
                    Marshal.Release(instance);
                }
                Marshal.Release(factory);
            }
        }

        private static unsafe IntPtr GetActivationFactory(string className)
        {
#if CSWINRT3_0
            return (IntPtr)global::WindowsRuntime.InteropServices.WindowsRuntimeActivationFactory.GetActivationFactoryUnsafe(className);
#else
            var factory = global::WinRT.ActivationFactory.Get(className);
            IntPtr pointer = factory.ThisPtr;
            Marshal.AddRef(pointer);
            GC.KeepAlive(factory);
            return pointer;
#endif
        }

        private static unsafe string GetRuntimeClassName(IntPtr unknown)
        {
            Guid iid = IID_IInspectable;
#if CSWINRT3_0
            Marshal.ThrowExceptionForHR(Marshal.QueryInterface(unknown, in iid, out IntPtr inspectable));
#else
            Marshal.ThrowExceptionForHR(Marshal.QueryInterface(unknown, ref iid, out IntPtr inspectable));
#endif
            IntPtr className = IntPtr.Zero;
            try
            {
                // Read the native name, not the managed projection or derived type name.
                var getName = (delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, int>)(*(void***)inspectable)[4];
                Marshal.ThrowExceptionForHR(getName(inspectable, &className));
                char* buffer = WindowsGetStringRawBuffer(className, out uint length);
                return new string(buffer, 0, checked((int)length));
            }
            finally
            {
                WindowsDeleteString(className);
                Marshal.Release(inspectable);
            }
        }

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern unsafe char* WindowsGetStringRawBuffer(IntPtr value, out uint length);

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern int WindowsDeleteString(IntPtr value);
    }
}
