// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;

using WPFMarkup = System.Windows.Markup;

using Microsoft.UI.Xaml.Tests.Common;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;
using Xaml = Microsoft.UI.Xaml;

using WPFControls = System.Windows.Controls;
using WPFSystem = System.Windows;
using System.Runtime.InteropServices.WindowsRuntime;

using Private.Infrastructure.Hosting.WPF;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    public static class WPFControlsExtensions
    {
        public static async Task FocusAndWaitAsync(this WPFControls.Control control)
        {
            if (control.IsFocused)
            {
                return;
            }

            var completeEvent = new TaskCompletionSource<bool>();
            WPFSystem.RoutedEventHandler handler = (o, a) =>
            {
                Log.Comment($"Focus is on {o}");
                completeEvent.SetResult(true);
            };
            control.GotFocus += handler;
            try
            {                
                control.Focus();
                await completeEvent.Task;
            }
            finally
            {
                control.GotFocus -= handler;
            }
        }

        public static async Task WaitGotFocusAsync(this WPFControls.Control control)
        {
            if (control.IsFocused)
            {
                return;
            }
            var completeEvent = new TaskCompletionSource<bool>();
            WPFSystem.RoutedEventHandler handler = (o, a) =>
            {
                // Seems like WPF control GetFocusedElement not returning valid element.
                Log.Comment($"Focus is on {o}");
                completeEvent.SetResult(true);
            };
            control.GotFocus += handler;
            try
            {
                await completeEvent.Task;
            }
            finally
            {
                control.GotFocus -= handler;
            }
        }

        public static async Task WaitFrameworkElementUnloadedAsync(this Xaml.FrameworkElement frameworkElement)
        {
            var completeEvent = new TaskCompletionSource<bool>();
            Xaml.RoutedEventHandler handler = (o, a) =>
            {
                Log.Comment($"Unloaded called on {o}");
                completeEvent.SetResult(true);
            };
            frameworkElement.Unloaded += handler;
            try
            {
                await completeEvent.Task;
                Verify.IsTrue(completeEvent.Task.Result);
            }
            finally
            {
                frameworkElement.Unloaded -= handler;
            }
        }

    }
}