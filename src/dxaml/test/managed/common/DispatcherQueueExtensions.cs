// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace Microsoft.UI.Dispatching
{
    public static class DispatcherQueueExtensions
    {
        public async static Task<T> TryEnqueueAsync<T>(this DispatcherQueue queue, Func<Task<T>> function, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal)
        {
            var completitionSource = new TaskCompletionSource<T>();
            //workaround for IAgileObject for lambda objects in WinRT
            DispatcherQueueHandler handler = global::Private.Infrastructure.WindowHelper.WrapInAgileDispatcherQueueHandler(
                new global::Private.Infrastructure.ManagedDispatcherQueueCallback(
                    async () =>
                    {
                        try
                        {
                            completitionSource.SetResult(await function());
                        }
                        catch (OperationCanceledException)
                        {
                            completitionSource.SetCanceled();
                        }
                        catch (Exception e)
                        {
                            completitionSource.SetException(e);
                        }
                    })
            );

            var inQueue = queue.TryEnqueue(priority, handler);
            if (!inQueue)
            {
                throw new InvalidOperationException();
            }
            var result = await completitionSource.Task;
            return result;
        }

        public async static Task TryEnqueueAsync(this DispatcherQueue queue, Action action, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal)
        {
            await queue.TryEnqueueAsync<int>(() => { action(); return Task.FromResult<int>(0); }, priority);
        }

        public async static Task TryEnqueueAsync(this DispatcherQueue queue, Func<Task> function, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal)
        {
            await queue.TryEnqueueAsync<int>(async () => { await function(); return 0; }, priority);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct FIXED {
            public short fract;
            public short value;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct POINT {
            public FIXED x;
            public FIXED y;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MSG
        {
            IntPtr hwnd;
            uint message;
            UIntPtr wParam;
            IntPtr lParam;
            int time;
            POINT pt;
        }

        [DllImport("user32.dll")]
        private static extern sbyte GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);
        [DllImport("user32.dll")]
        private static extern bool TranslateMessage([In] ref MSG lpMsg);
        [DllImport("user32.dll")]
        private static extern IntPtr DispatchMessage([In] ref MSG lpmsg);

        public static void DoEvents(this DispatcherQueue queue, DispatcherQueuePriority priority = DispatcherQueuePriority.Normal)
        {
            var task = queue.TryEnqueueAsync(() => Task.FromResult(0));
            MSG msg;
            while (!task.IsCompleted && !task.IsFaulted && !task.IsCanceled && GetMessage(out msg, IntPtr.Zero, 0, 0) > 0)
            {
                TranslateMessage(ref msg);
                DispatchMessage(ref msg);
            }
        }
    }
}
