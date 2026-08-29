// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using MS.Win32;
using Xaml = Microsoft.UI.Xaml;

namespace Microsoft.Windows.Interop
{
    // TODO: Partition Focus and Input into XamlContentHostKeyboardFocus and XamlContentHostInput files

    /// <summary>
    ///     A Windows Forms control that can be used to host XAML content
    /// </summary>
    partial class XamlContentHost
    {
        /// <summary>
        ///     Does this Control currently have focus? Check both the Control's 
        ///     window handle and the hosted Xaml window handle. If either has focus
        ///     then this Control currently has focus. 
        /// </summary>
        public override bool Focused
        {
            get
            {
                return this.xamlBridge.HasFocus;
            }
        }

        protected override void OnGotFocus(EventArgs e)
        {
            base.OnGotFocus(e);

            if (!this.xamlBridge.HasFocus)
            {
                this.xamlBridge.NavigateFocus(new Xaml.Hosting.XamlSourceFocusNavigationRequest(Xaml.Hosting.XamlSourceFocusNavigationReason.Programmatic));
            }
        }

        protected override void OnClick(EventArgs e)
        {
            (this.Parent as IContainerControl).ActivateControl(this);
            base.OnClick(e);
        }

        [global::System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern int OemKeyScan(short wAsciiVal);

        private int GetLParamForKeyDown(int character)
        {
            int lParam = 0x00000001;
            int oemVal = OemKeyScan((short)(0xFF & character));
            if (oemVal != -1)
            {
                oemVal <<= 16;
                lParam += oemVal;
            }
            return lParam;
        }

        private int GetLParamForKeyUp(int character)
        {
            int lParam = unchecked((int)0xC0000001);
            int oemVal = OemKeyScan((short)(0xFF & character));
            if (oemVal != -1)
            {
                oemVal <<= 16;
                lParam += oemVal;
            }
            return (int)lParam;
        }

        private bool ProcessTabKey(bool forward)
        {
            if (!this.xamlBridge.HasFocus)
            {
                var reason = forward ? Xaml.Hosting.XamlSourceFocusNavigationReason.First : Xaml.Hosting.XamlSourceFocusNavigationReason.Last;
                var result = this.xamlBridge.NavigateFocus(new Xaml.Hosting.XamlSourceFocusNavigationRequest(reason));
                if (result.WasFocusMoved)
                {
                    return true;
                }
                return false;
            }
            else
            {
                var hWnd = UnsafeNativeMethods.GetFocus();
                var result = UnsafeNativeMethods.SendMessage(new HandleRef(this, hWnd), NativeMethods.WM_KEYDOWN, new IntPtr((int)Keys.Tab), new IntPtr(GetLParamForKeyDown((int)Keys.Tab)));
                result = UnsafeNativeMethods.SendMessage(new HandleRef(this, hWnd), NativeMethods.WM_KEYUP, new IntPtr((int)Keys.Tab), new IntPtr(GetLParamForKeyUp((int)Keys.Tab)));
                return result.ToInt32() == 1;
            }
        }

        protected override void Select(bool directed, bool forward)
        {
            this.ProcessTabKey(forward);
        }

        private void XamlBridge_TakeFocusRequested(Xaml.Hosting.DesktopWindowXamlSource sender, Xaml.Hosting.DesktopWindowXamlSourceTakeFocusRequestedEventArgs args)
        {
            var reason = args.Request.Reason;
            if (reason == Xaml.Hosting.XamlSourceFocusNavigationReason.First || reason == Xaml.Hosting.XamlSourceFocusNavigationReason.Last)
            {
                var forward = reason == Xaml.Hosting.XamlSourceFocusNavigationReason.First;
                this.Parent.SelectNextControl(this, forward, tabStopOnly: true, nested: false, wrap: true);
            }
        }
    }
}
