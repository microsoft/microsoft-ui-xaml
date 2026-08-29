// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.UI.Xaml
{
    internal static class FrameworkElementExtensions
    {
        public static async Task AwaitLoaded(this FrameworkElement element)
        {
            var completeEvent = new TaskCompletionSource<bool>();
            RoutedEventHandler handler = (o, a) =>
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

        public static async Task AwaitUnloaded(this FrameworkElement element)
        {
            var completeEvent = new TaskCompletionSource<bool>();
            RoutedEventHandler handler = (o, a) =>
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

