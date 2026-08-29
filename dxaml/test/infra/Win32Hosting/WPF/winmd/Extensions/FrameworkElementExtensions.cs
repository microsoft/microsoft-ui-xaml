// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WPFMarkup = System.Windows.Markup;
using WPFControls = System.Windows.Controls;
using WPFSystem = System.Windows;
using System.Runtime.InteropServices.WindowsRuntime;

namespace System.Windows
{
    internal static class FrameworkElementExtensions
    {
        public static async Task AwaitLoaded(this WPFSystem.FrameworkElement element)
        {
            var completeEvent = new TaskCompletionSource<bool>();
            WPFSystem.RoutedEventHandler handler = (o, a) =>
            {
                completeEvent.SetResult(true);
            };
            element.Loaded += handler;
            try
            {
                if (element.IsLoaded)
                {
                    return;
                }
                await completeEvent.Task;
            }
            finally
            {
                element.Loaded -= handler;
            }
        }

        public static async Task AwaitUnloaded(this WPFSystem.FrameworkElement element)
        {
            var completeEvent = new TaskCompletionSource<bool>();
            WPFSystem.RoutedEventHandler handler = (o, a) =>
            {
                completeEvent.SetResult(true);
            };
            element.Unloaded += handler;
            try
            {
                if (!element.IsLoaded)
                {
                    return;
                }
                await completeEvent.Task;
            }
            finally
            {
                element.Unloaded -= handler;
            }
        }
    }
}

