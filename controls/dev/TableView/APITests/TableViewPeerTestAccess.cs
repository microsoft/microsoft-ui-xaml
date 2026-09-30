// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using WinRT;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    internal static class TableViewPeerTestAccess
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int ExistingPeerGetter(IntPtr instance, out IntPtr peer);

        public static bool HasExistingPeer(UIElement cell)
        {
            // Internal read-only interface in TableViewCell.h; FromElement creates
            // a peer in this framework and cannot be used for this assertion.
            var iid = new Guid("843c3f59-aab0-4bcb-a8a4-12327286a29f");
            IntPtr access = IntPtr.Zero, peer = IntPtr.Zero;
            try
            {
                Marshal.ThrowExceptionForHR(Marshal.QueryInterface(
                    ((IWinRTObject)cell).NativeObject.ThisPtr, ref iid, out access));
                var vtable = Marshal.ReadIntPtr(access);
                var method = Marshal.ReadIntPtr(vtable, 3 * IntPtr.Size);
                var getExisting = Marshal.GetDelegateForFunctionPointer<ExistingPeerGetter>(method);
                Marshal.ThrowExceptionForHR(getExisting(access, out peer));
                return peer != IntPtr.Zero;
            }
            finally
            {
                if (peer != IntPtr.Zero) Marshal.Release(peer);
                if (access != IntPtr.Zero) Marshal.Release(access);
            }
        }
    }
}
