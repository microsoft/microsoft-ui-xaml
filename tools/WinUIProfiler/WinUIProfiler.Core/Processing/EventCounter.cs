// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Counts events per provider; the app shows the totals while connected.
public sealed class EventCounter : IEventProcessor
{
    private long xamlEvents;
    private long profilerEvents;

    public long XamlEvents => Interlocked.Read(ref xamlEvents);
    public long ProfilerEvents => Interlocked.Read(ref profilerEvents);

    public void Process(long sequence, ProfilerEvent profilerEvent)
    {
        if (profilerEvent.Provider == XamlProvider.Xaml)
        {
            Interlocked.Increment(ref xamlEvents);
        }
        else
        {
            Interlocked.Increment(ref profilerEvents);
        }
    }
}
