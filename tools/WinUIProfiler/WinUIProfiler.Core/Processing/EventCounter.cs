// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Counts events by provider; the app shows the totals while connected. XamlEvents counts the XAML providers' events
// except Microsoft-Windows-XAML-Profiler's, which ProfilerEvents counts.
public sealed class EventCounter : IEventProcessor
{
    private long xamlEvents;
    private long profilerEvents;
    private long gpuMemoryEvents;

    public long XamlEvents => Interlocked.Read(ref xamlEvents);
    public long ProfilerEvents => Interlocked.Read(ref profilerEvents);
    public long GpuMemoryEvents => Interlocked.Read(ref gpuMemoryEvents);

    public void Process(long sequence, ProfilerEvent profilerEvent)
    {
        switch (profilerEvent.Provider)
        {
            case XamlProvider.Xaml:
            case XamlProvider.XamlDiagnostics:
            case XamlProvider.MicrosoftUIXaml:
            case XamlProvider.MicrosoftUIXamlControls:
                Interlocked.Increment(ref xamlEvents);
                break;
            case XamlProvider.XamlProfiler:
                Interlocked.Increment(ref profilerEvents);
                break;
            case XamlProvider.DxgKrnl:
                Interlocked.Increment(ref gpuMemoryEvents);
                break;
        }
    }
}
