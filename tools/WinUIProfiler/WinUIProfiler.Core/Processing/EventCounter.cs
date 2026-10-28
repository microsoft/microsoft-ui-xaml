// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Counts events per provider; the app shows the totals while connected.
public sealed class EventCounter : IEventProcessor
{
    private readonly long[] counts = new long[Enum.GetValues<XamlProvider>().Length];

    public long Count(XamlProvider provider) => Interlocked.Read(ref counts[(int)provider]);

    public void Process(long sequence, ProfilerEvent profilerEvent) =>
        Interlocked.Increment(ref counts[(int)profilerEvent.Provider]);
}
