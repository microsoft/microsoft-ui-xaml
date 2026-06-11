// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using Microsoft.UI.Dispatching;
using System.Diagnostics;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class UIExecutor
    {
        private static readonly TimeSpan defaultTimeOut = TimeSpan.FromMinutes(5);

        public static void Execute(Action operation)
        {
            if (global::Private.Infrastructure.TestServices.WindowHelper.CurrentDispatcher.HasThreadAccess)
            {
                operation();
            }
            else
            {
                var task = global::Private.Infrastructure.TestServices.WindowHelper.CurrentDispatcher.TryEnqueueAsync(operation);

                if(Debugger.IsAttached)
                {
                    task.Wait();
                }
                else if (!task.Wait(defaultTimeOut))
                {
                    throw new TimeoutException("UI operation timeout, UI maybe hung or App was suspended");
                }
            }
        }

        public static async Task<T> ExecuteAsync<T>(Func<Task<T>> operation)
        {
            var uiTask = default(Task<T>);

            if (global::Private.Infrastructure.TestServices.WindowHelper.CurrentDispatcher.HasThreadAccess)
            {
                uiTask = operation();
            }
            else
            {
                uiTask = global::Private.Infrastructure.TestServices.WindowHelper.CurrentDispatcher.TryEnqueueAsync(operation);
            }

            if (await Task.WhenAny(uiTask, Task.Delay(defaultTimeOut)) == uiTask)
            {
                return uiTask.Result;
            }
            else
            {
                throw new TimeoutException("UI operation timeout, UI maybe hung or App was suspended");
            }
        }

        public static async Task ExecuteAsync(Func<Task> operation) =>
            await ExecuteAsync(async () => { await operation(); return false; });
    }
}
