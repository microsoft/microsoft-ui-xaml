// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Windows;
// CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
// Due to namespace clashing I had to use variable names with them.
using Microsoft.Toolkit.Win32.UI.XamlHost;
using WUX = Microsoft.UI.Xaml;
using WF = Windows.Foundation;
using WinRT;
// CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

namespace Microsoft.Toolkit.Wpf.UI.XamlHost
{
    /// <summary>
    /// Focus and Keyboard handling for Focus integration with UWP XAML
    /// </summary>
    partial class WindowsXamlHostBase
    {
        /// <summary>
        /// Dictionary that maps WPF (host framework) FocusNavigationDirection to UWP XAML XxamlSourceFocusNavigationReason
        /// </summary>
        private static readonly Dictionary<global::System.Windows.Input.FocusNavigationDirection, WUX.Hosting.XamlSourceFocusNavigationReason>
            MapDirectionToReason =
                new Dictionary<global::System.Windows.Input.FocusNavigationDirection, WUX.Hosting.XamlSourceFocusNavigationReason>
                {
                    { global::System.Windows.Input.FocusNavigationDirection.Next,     WUX.Hosting.XamlSourceFocusNavigationReason.First },
                    { global::System.Windows.Input.FocusNavigationDirection.First,    WUX.Hosting.XamlSourceFocusNavigationReason.First },
                    { global::System.Windows.Input.FocusNavigationDirection.Previous, WUX.Hosting.XamlSourceFocusNavigationReason.Last },
                    { global::System.Windows.Input.FocusNavigationDirection.Last,     WUX.Hosting.XamlSourceFocusNavigationReason.Last },
                    { global::System.Windows.Input.FocusNavigationDirection.Up,       WUX.Hosting.XamlSourceFocusNavigationReason.Up },
                    { global::System.Windows.Input.FocusNavigationDirection.Down,     WUX.Hosting.XamlSourceFocusNavigationReason.Down },
                    { global::System.Windows.Input.FocusNavigationDirection.Left,     WUX.Hosting.XamlSourceFocusNavigationReason.Left },
                    { global::System.Windows.Input.FocusNavigationDirection.Right,    WUX.Hosting.XamlSourceFocusNavigationReason.Right },
                };

        /// <summary>
        /// Dictionary that maps UWP XAML XamlSourceFocusNavigationReason to WPF (host framework) FocusNavigationDirection
        /// </summary>
        private static readonly Dictionary<WUX.Hosting.XamlSourceFocusNavigationReason, global::System.Windows.Input.FocusNavigationDirection>
            MapReasonToDirection =
                new Dictionary<WUX.Hosting.XamlSourceFocusNavigationReason, global::System.Windows.Input.FocusNavigationDirection>()
                {
                    { WUX.Hosting.XamlSourceFocusNavigationReason.First, global::System.Windows.Input.FocusNavigationDirection.Next },
                    { WUX.Hosting.XamlSourceFocusNavigationReason.Last,  global::System.Windows.Input.FocusNavigationDirection.Previous },
                    { WUX.Hosting.XamlSourceFocusNavigationReason.Up,    global::System.Windows.Input.FocusNavigationDirection.Up },
                    { WUX.Hosting.XamlSourceFocusNavigationReason.Down,  global::System.Windows.Input.FocusNavigationDirection.Down },
                    { WUX.Hosting.XamlSourceFocusNavigationReason.Left,  global::System.Windows.Input.FocusNavigationDirection.Left },
                    { WUX.Hosting.XamlSourceFocusNavigationReason.Right, global::System.Windows.Input.FocusNavigationDirection.Right },
                };

        /// <summary>
        /// Last Focus Request GUID to uniquely identify Focus operations, primarily used with error callbacks
        /// </summary>
        private Guid _lastFocusRequest = Guid.Empty;

        /// <summary>
        /// Override for OnGotFocus that passes NavigateFocus on to the DesktopXamlSource instance
        /// </summary>
        /// <param name="e">RoutedEventArgs</param>
        protected override void OnGotFocus(RoutedEventArgs e)
        {
            base.OnGotFocus(e);

            if (!_xamlSource.HasFocus)
            {
                _xamlSource.NavigateFocus(
                    new WUX.Hosting.XamlSourceFocusNavigationRequest(
                        WUX.Hosting.XamlSourceFocusNavigationReason.Programmatic));
            }
        }

        /// <summary>
        /// Process Tab from host framework
        /// </summary>
        /// <param name="request"><see cref="System.Windows.Input.TraversalRequest"/> that contains requested navigation direction</param>
        /// <returns>Did handle tab</returns>
        protected override bool TabIntoCore(global::System.Windows.Input.TraversalRequest request)
        {
            if (_xamlSource.HasFocus && !_onTakeFocusRequested)
            {
                return false; // If we have focus already, then we dont need to NavigateFocus
            }

            // Focus is wrong if the previous element is in a different FocusScope than the WindowsXamlHost element.
            var focusedElement = global::System.Windows.Input.FocusManager.GetFocusedElement(
                global::System.Windows.Input.FocusManager.GetFocusScope(this)) as FrameworkElement;

            var origin = BoundsRelativeTo(focusedElement, this);
            var reason = MapDirectionToReason[request.FocusNavigationDirection];
            if (_lastFocusRequest == Guid.Empty)
            {
                _lastFocusRequest = Guid.NewGuid();
            }

            var sourceFocusNavigationRequest = new WUX.Hosting.XamlSourceFocusNavigationRequest(reason, origin, _lastFocusRequest);
            try
            {
                var result = _xamlSource.NavigateFocus(sourceFocusNavigationRequest);

                // Returning true indicates that focus moved.  This will cause the HwndHost to
                // move focus to the source's hwnd (call SetFocus Win32 API)
                return result.WasFocusMoved;
            }
            finally
            {
                _lastFocusRequest = Guid.Empty;
            }
        }

        /// <summary>
        /// Transform bounds relative to FrameworkElement
        /// </summary>
        /// <param name="sibling1">base rectangle</param>
        /// <param name="sibling2">second of pair to transform</param>
        /// <returns>result of transformed rectangle</returns>
        private static WF.Rect BoundsRelativeTo(FrameworkElement sibling1, global::System.Windows.Media.Visual sibling2)
        {
            WF.Rect origin = default(WF.Rect);

            if (sibling1 != null)
            {
                var transform = sibling1.TransformToVisual(sibling2);
                var systemWindowsRect = transform.TransformBounds(
                    new Rect(0, 0, sibling1.ActualWidth, sibling1.ActualHeight));
                origin.X = systemWindowsRect.X;
                origin.Y = systemWindowsRect.Y;
                origin.Width = systemWindowsRect.Width;
                origin.Height = systemWindowsRect.Height;
            }

            return origin;
        }

        private bool _onTakeFocusRequested = false;

        /// <summary>
        /// Handles the <see cref="E:TakeFocusRequested" /> event.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSourceTakeFocusRequestedEventArgs"/> instance containing the event data.</param>
        private void OnTakeFocusRequested(object sender, WUX.Hosting.DesktopWindowXamlSourceTakeFocusRequestedEventArgs e)
        {
            if (_lastFocusRequest == e.Request.CorrelationId)
            {
                // If we've arrived at this point, then focus is being move back to us
                // therefore, we should complete the operation to avoid an infinite recursion
                // by "Restoring" the focus back to us under a new correctationId
                var newRequest = new WUX.Hosting.XamlSourceFocusNavigationRequest(
                    WUX.Hosting.XamlSourceFocusNavigationReason.Restore);
                _xamlSource.NavigateFocus(newRequest);
            }
            else
            {
                _onTakeFocusRequested = true;
                try
                {
                    // NOTE: The behavior here is specifically to help Xaml tests work well, it's NOT
                    // meant to be used as a general best-practice for WPF hosting Xaml.  Please 
                    // see Microsoft.Toolkit.Wpf.UI.XamlHost for a better generic handler for
                    // TakeFocusRequested.
                    // Tab-cycle through the Xaml content.  Tabbing from last element will cycle to
                    // the first Xaml element, etc.  Directional/XY focus movements will not cycle,
                    // they'll just stop when there are no elements in the island in that direction.
                    if (e.Request.Reason == WUX.Hosting.XamlSourceFocusNavigationReason.First 
                        || e.Request.Reason == WUX.Hosting.XamlSourceFocusNavigationReason.Last) 
                    {                    
                        var newRequest = new WUX.Hosting.XamlSourceFocusNavigationRequest(
                            e.Request.Reason);
                        _lastFocusRequest = newRequest.CorrelationId;
                        _xamlSource.NavigateFocus(newRequest);
                    }
                }
                finally
                {
                    _onTakeFocusRequested = false;
                }
            }
        }

        private void OnThreadFilterMessage(ref global::System.Windows.Interop.MSG msg, ref bool handled)
        {
            if (!handled)
            {
                handled = Private.Infrastructure.Hosting.WPF.Interop.ContentPreTranslateMessage(ref msg);
            }
        }

        protected override bool HasFocusWithinCore()
        {
            return _xamlSource.HasFocus;
        }
    }
}
