// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Runtime.InteropServices;
using WinRT;

namespace Microsoft.UI.Xaml.Tests.Framework.ApiSurface
{
    // Thin, test-only access to the raw IInspectable/IUnknown ABI of objects and activation factories that live in
    // Microsoft.UI.Xaml.dll. The C# projection hides IInspectable::GetIids, so the sweep tests call the vtable
    // directly (slot 0 QueryInterface, 2 Release, 3 GetIids, 4 GetRuntimeClassName) through Marshal delegates.
    internal static class InspectableAbiHelper
    {
        public const string MuxModuleName = "Microsoft.UI.Xaml.dll";

        public const int E_NOTIMPL = unchecked((int)0x80004001);
        public const int E_NOINTERFACE = unchecked((int)0x80004002);
        public const int E_FAIL = unchecked((int)0x80004005);
        public const int E_POINTER = unchecked((int)0x80004003);
        public const int E_ACCESSDENIED = unchecked((int)0x80070005);
        public const int E_INVALIDARG = unchecked((int)0x80070057);
        public const int CLASS_E_CLASSNOTAVAILABLE = unchecked((int)0x80040111);
        public const int RPC_E_WRONG_THREAD = unchecked((int)0x8001010E);
        public const int E_XAMLPARSEFAILED = unchecked((int)0x802B000A);

        public static readonly Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");
        public static readonly Guid IID_IInspectable = new Guid("AF86E2E0-B12D-4C6A-9C5A-D7AA65101E90");
        public static readonly Guid IID_IActivationFactory = new Guid("00000035-0000-0000-C000-000000000046");

        // A GUID that no Microsoft.UI.Xaml object implements; used for the negative QueryInterface contract.
        public static readonly Guid IID_NotImplemented = new Guid("6A1C5E2B-90F3-4C7E-B1D4-3F2A8E9C0D57");

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int QueryInterfaceFn(IntPtr self, ref Guid iid, ref IntPtr ppv);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate uint ReleaseFn(IntPtr self);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int GetIidsFn(IntPtr self, out uint iidCount, out IntPtr iids);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int GetRuntimeClassNameFn(IntPtr self, out IntPtr className);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int DllGetActivationFactoryFn(IntPtr activatableClassId, out IntPtr factory);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr GetModuleHandleW(string moduleName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr module, string procName);

        [DllImport("combase.dll", PreserveSig = true)]
        private static extern int WindowsCreateString([MarshalAs(UnmanagedType.LPWStr)] string sourceString, int length, out IntPtr hstring);

        [DllImport("combase.dll", PreserveSig = true)]
        private static extern int WindowsDeleteString(IntPtr hstring);

        [DllImport("combase.dll", PreserveSig = true)]
        private static extern IntPtr WindowsGetStringRawBuffer(IntPtr hstring, out uint length);

        private static DllGetActivationFactoryFn s_dllGetActivationFactory;

        // Non-generic on purpose: a generic Delegate-constrained helper crashes the CsWinRT AOT source generator
        // (CS8785), which would silently drop its output for the whole test project.
        private static Delegate GetVtableFunction(IntPtr comPointer, int slot, Type delegateType)
        {
            IntPtr vtable = Marshal.ReadIntPtr(comPointer);
            IntPtr function = Marshal.ReadIntPtr(vtable, slot * IntPtr.Size);
            return Marshal.GetDelegateForFunctionPointer(function, delegateType);
        }

        // Non-null value written to *ppv before every QueryInterface call, so that an implementation that does not write
        // *ppv is observable. It is never dereferenced or released.
        public static readonly IntPtr QueryInterfaceOutSentinel = new IntPtr(0x5EED0BAD);

        // COM requires QueryInterface to set *ppv to null on failure and to a valid pointer on success. *ppv starts as
        // QueryInterfaceOutSentinel. A failed call's out pointer is never owned by the caller, so it is reported through
        // 'outPointerOnFailure' (null when the contract is met; the sentinel when *ppv was left unwritten) and 'result' is
        // null: releasing it would over-release the object. A successful call that leaves the sentinel in *ppv also reports
        // it through 'outPointerOnFailure', with a null 'result'.
        public static int QueryInterface(IntPtr comPointer, Guid iid, out IntPtr result, out IntPtr outPointerOnFailure)
        {
            var queryInterface = (QueryInterfaceFn)GetVtableFunction(comPointer, 0, typeof(QueryInterfaceFn));
            IntPtr ppv = QueryInterfaceOutSentinel;
            int hr = queryInterface(comPointer, ref iid, ref ppv);
            result = ppv;
            outPointerOnFailure = IntPtr.Zero;
            if (hr < 0 || ppv == QueryInterfaceOutSentinel)
            {
                outPointerOnFailure = ppv;
                result = IntPtr.Zero;
            }
            return hr;
        }

        // For QueryInterface calls that must succeed: throws on failure, including the out-pointer contract violation.
        private static IntPtr QueryInterfaceOrThrow(IntPtr comPointer, Guid iid)
        {
            int hr = QueryInterface(comPointer, iid, out IntPtr result, out IntPtr outPointerOnFailure);
            if (outPointerOnFailure != IntPtr.Zero)
            {
                throw new InvalidOperationException(outPointerOnFailure == QueryInterfaceOutSentinel
                    ? $"QueryInterface({{{iid}}}) returned 0x{hr:X8} without writing its out pointer."
                    : $"QueryInterface({{{iid}}}) failed with 0x{hr:X8} but left a non-null out pointer.");
            }
            Marshal.ThrowExceptionForHR(hr);
            return result;
        }

        public static void Release(IntPtr comPointer)
        {
            if (comPointer != IntPtr.Zero)
            {
                var release = (ReleaseFn)GetVtableFunction(comPointer, 2, typeof(ReleaseFn));
                release(comPointer);
            }
        }

        // Returns the HRESULT of IInspectable::GetIids; on success 'iids' holds the copied array.
        public static int GetIids(IntPtr inspectable, out Guid[] iids)
        {
            iids = Array.Empty<Guid>();
            var getIids = (GetIidsFn)GetVtableFunction(inspectable, 3, typeof(GetIidsFn));
            int hr = getIids(inspectable, out uint count, out IntPtr buffer);
            if (hr >= 0)
            {
                try
                {
                    iids = new Guid[count];
                    int guidSize = Marshal.SizeOf(typeof(Guid));
                    for (int i = 0; i < count; i++)
                    {
                        iids[i] = (Guid)Marshal.PtrToStructure(buffer + (i * guidSize), typeof(Guid));
                    }
                }
                finally
                {
                    Marshal.FreeCoTaskMem(buffer);
                }
            }
            return hr;
        }

        // Returns the HRESULT of IInspectable::GetRuntimeClassName; on success 'className' holds the name.
        public static int GetRuntimeClassName(IntPtr inspectable, out string className)
        {
            return GetRuntimeClassName(inspectable, out className, out bool _);
        }

        // As above, also reporting whether the returned HSTRING was null (the WinRT encoding of an empty name).
        public static int GetRuntimeClassName(IntPtr inspectable, out string className, out bool isNullHString)
        {
            className = null;
            isNullHString = false;
            var getRuntimeClassName = (GetRuntimeClassNameFn)GetVtableFunction(inspectable, 4, typeof(GetRuntimeClassNameFn));
            int hr = getRuntimeClassName(inspectable, out IntPtr hstring);
            if (hr >= 0)
            {
                isNullHString = hstring == IntPtr.Zero;
                className = HStringToString(hstring);
                WindowsDeleteString(hstring);
            }
            return hr;
        }

        private static string HStringToString(IntPtr hstring)
        {
            if (hstring == IntPtr.Zero)
            {
                return string.Empty;
            }
            IntPtr raw = WindowsGetStringRawBuffer(hstring, out uint length);
            return Marshal.PtrToStringUni(raw, (int)length);
        }

        // Calls Microsoft.UI.Xaml.dll!DllGetActivationFactory directly so that type ownership is decided by the DLL
        // under test rather than by the process's activation registrations. A returned factory is AddRef'd.
        public static int GetMuxActivationFactory(string runtimeClassName, out IntPtr factory)
        {
            factory = IntPtr.Zero;
            if (s_dllGetActivationFactory == null)
            {
                IntPtr module = GetModuleHandleW(MuxModuleName);
                if (module == IntPtr.Zero)
                {
                    throw new InvalidOperationException($"{MuxModuleName} is not loaded in the test process.");
                }
                IntPtr export = GetProcAddress(module, "DllGetActivationFactory");
                if (export == IntPtr.Zero)
                {
                    throw new InvalidOperationException($"{MuxModuleName} does not export DllGetActivationFactory.");
                }
                s_dllGetActivationFactory = (DllGetActivationFactoryFn)Marshal.GetDelegateForFunctionPointer(export, typeof(DllGetActivationFactoryFn));
            }

            int hr = WindowsCreateString(runtimeClassName, runtimeClassName.Length, out IntPtr hstring);
            if (hr < 0)
            {
                return hr;
            }
            try
            {
                return s_dllGetActivationFactory(hstring, out factory);
            }
            finally
            {
                WindowsDeleteString(hstring);
            }
        }

        // Returns an AddRef'd IInspectable pointer for a projected WinRT object; throws for a non-WinRT object.
        public static IntPtr GetInspectable(object projected)
        {
            if (!(projected is IWinRTObject winrtObject) || winrtObject.NativeObject == null)
            {
                throw new ArgumentException($"{projected?.GetType().FullName ?? "null"} is not a projected WinRT object.");
            }
            IntPtr inspectable = QueryInterfaceOrThrow(winrtObject.NativeObject.ThisPtr, IID_IInspectable);
            GC.KeepAlive(projected);
            return inspectable;
        }

        // COM identity (canonical IUnknown) of a projected object, or IntPtr.Zero for a purely managed object.
        // The pointer value is only meaningful while 'projected' is alive.
        public static IntPtr GetIdentity(object projected)
        {
            if (projected is IWinRTObject winrtObject && winrtObject.NativeObject != null)
            {
                IntPtr unknown = QueryInterfaceOrThrow(winrtObject.NativeObject.ThisPtr, IID_IUnknown);
                Release(unknown);
                GC.KeepAlive(projected);
                return unknown;
            }
            return IntPtr.Zero;
        }
    }
}
