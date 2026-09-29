// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Windows.System;
using WinRT;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    internal static class TableViewPeerTestAccess
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int ExistingPeerGetter(IntPtr instance, out IntPtr peer);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int CellCursorMover(IntPtr instance, int virtualKey,
            [MarshalAs(UnmanagedType.U1)] bool control, [MarshalAs(UnmanagedType.U1)] out bool handled);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int AnchoredCellCursorMover(IntPtr instance, int virtualKey,
            [MarshalAs(UnmanagedType.U1)] bool control, int anchorRow, int anchorColumn,
            [MarshalAs(UnmanagedType.U1)] out bool handled);

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

        /// <summary>
        /// Drives the owning TableView's cell cursor for a virtual key, through exactly the code its
        /// KeyDown handler runs. API tests run in-process and cannot synthesize a KeyRoutedEventArgs,
        /// so ITableViewCellNavigationAccess in TableViewCell.h is the seam they use instead.
        /// Returns whether the key belonged to cell navigation.
        /// </summary>
        public static bool MoveCellCursor(UIElement cell, VirtualKey key, bool control = false)
        {
            var iid = new Guid("6b0d94f1-3c27-4e58-8a6d-71f2c9b40e35");
            IntPtr access = IntPtr.Zero;
            try
            {
                Marshal.ThrowExceptionForHR(Marshal.QueryInterface(
                    ((IWinRTObject)cell).NativeObject.ThisPtr, ref iid, out access));
                var vtable = Marshal.ReadIntPtr(access);
                var method = Marshal.ReadIntPtr(vtable, 3 * IntPtr.Size);
                var move = Marshal.GetDelegateForFunctionPointer<CellCursorMover>(method);
                Marshal.ThrowExceptionForHR(move(access, (int)key, control, out bool handled));
                return handled;
            }
            finally
            {
                if (access != IntPtr.Zero) Marshal.Release(access);
            }
        }

        /// <summary>
        /// Delivers the key with an explicit pre-key cursor anchor, the way the control's tunneling
        /// PreviewKeyDown snapshot feeds its bubbling KeyDown handler. Lets a test reproduce the
        /// condition that made one arrow press move two columns: the framework's built-in
        /// directional navigation having already advanced focus before the handler runs.
        /// </summary>
        public static bool MoveCellCursorFromAnchor(
            UIElement cell, VirtualKey key, int anchorRow, int anchorColumn, bool control = false)
        {
            var iid = new Guid("6b0d94f1-3c27-4e58-8a6d-71f2c9b40e35");
            IntPtr access = IntPtr.Zero;
            try
            {
                Marshal.ThrowExceptionForHR(Marshal.QueryInterface(
                    ((IWinRTObject)cell).NativeObject.ThisPtr, ref iid, out access));
                var vtable = Marshal.ReadIntPtr(access);
                // Slot 4: IUnknown occupies 0-2, MoveCellCursor is 3.
                var method = Marshal.ReadIntPtr(vtable, 4 * IntPtr.Size);
                var move = Marshal.GetDelegateForFunctionPointer<AnchoredCellCursorMover>(method);
                Marshal.ThrowExceptionForHR(
                    move(access, (int)key, control, anchorRow, anchorColumn, out bool handled));
                return handled;
            }
            finally
            {
                if (access != IntPtr.Zero) Marshal.Release(access);
            }
        }
    }
}
