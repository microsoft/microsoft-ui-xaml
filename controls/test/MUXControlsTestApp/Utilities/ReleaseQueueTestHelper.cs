// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp.Utilities
{
    internal static class ReleaseQueueTestHelper
    {
        public static async Task RunBackgroundGCAsync(DispatcherQueue dispatcher)
        {
            var references = new List<WeakReference>();
            var collections = GC.CollectionCount(2);

            // Run GC off the UI thread while native cleanup wakes and subsequent batches
            // compete with reference tracking. Never request rendering or flush the queue.
            await Task.Run(async () =>
            {
                for (int batch = 0; batch < 32; batch++)
                {
                    await EnqueueAsync(dispatcher, () => CreateCycles(references)).ConfigureAwait(false);
                    if (dispatcher.HasThreadAccess)
                    {
                        throw new InvalidOperationException("GC must run on a background thread.");
                    }
                    // The CLR may choose a blocking collection even for a nonblocking request.
                    GC.Collect(2, GCCollectionMode.Forced, blocking: false);
                    await Task.Delay(1).ConfigureAwait(false);
                }

                // A native cleanup wake can unpeg a managed owner after the current GC.
                // Allow later collections to observe that, without blocking the UI thread.
                var timeout = Stopwatch.StartNew();
                do
                {
                    GC.Collect(2, GCCollectionMode.Forced, blocking: true);
                    GC.WaitForPendingFinalizers();
                    await EnqueueAsync(dispatcher, () => { }).ConfigureAwait(false);
                    await Task.Delay(10).ConfigureAwait(false);
                }
                while (HasSurvivors(references) && timeout.Elapsed < TimeSpan.FromSeconds(30));
            }).ConfigureAwait(false);

            if (GC.CollectionCount(2) <= collections)
            {
                throw new InvalidOperationException("The background worker did not complete a generation-2 GC.");
            }
            if (HasSurvivors(references))
            {
                throw new InvalidOperationException("Release-queue cycles survived background GC and dispatcher progress.");
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void CreateCycles(List<WeakReference> references)
        {
            for (int item = 0; item < 128; item++)
            {
                var owner = new CycleOwner();
                references.Add(new WeakReference(owner));
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool HasSurvivors(List<WeakReference> references)
        {
            foreach (var reference in references)
            {
                if (reference.IsAlive)
                {
                    return true;
                }
            }
            return false;
        }

        private static async Task EnqueueAsync(DispatcherQueue dispatcher, Action action)
        {
            var completion = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            if (!dispatcher.TryEnqueue(() =>
            {
                try
                {
                    action();
                    completion.SetResult(true);
                }
                catch (Exception error)
                {
                    completion.SetException(error);
                }
            }))
            {
                throw new InvalidOperationException("The UI dispatcher rejected the release-queue test callback.");
            }
            if (await Task.WhenAny(completion.Task, Task.Delay(TimeSpan.FromSeconds(30))).ConfigureAwait(false) != completion.Task)
            {
                throw new TimeoutException("The UI dispatcher stalled during background GC.");
            }
            await completion.Task.ConfigureAwait(false);
        }

        private sealed class CycleOwner
        {
            private readonly Grid grid;

            public CycleOwner()
            {
                // The native Tag tracker reference closes a native -> managed -> native cycle.
                // Reference tracking must put the unreachable Grid on the UI cleanup queue.
                grid = new Grid();
                grid.Tag = this;
            }
        }
    }
}
