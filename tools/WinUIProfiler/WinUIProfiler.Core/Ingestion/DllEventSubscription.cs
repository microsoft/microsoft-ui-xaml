// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

internal static class DllEventSubscription
{
    internal static void SubscribeKernel(ETWTraceEventSource source, Func<int, long, bool> isTarget,
        Action<ProfilerEvent> sink, bool includeImages)
    {
        var kernel = source.Kernel;
        kernel.MemoryHardFault += data =>
        {
            if (isTarget(data.ProcessID, data.TimeStamp.ToFileTimeUtc()))
            {
                sink(Capture(data, XamlProvider.KernelMemory, [new("VirtualAddress", data.VirtualAddress)]));
            }
        };

        if (includeImages)
        {
            // WPR's kernel loader events use the classic schema; the live session uses Kernel-Process.
            kernel.ImageLoad += Image;
            kernel.ImageUnload += Image;
        }

        void Image(ImageLoadTraceData data)
        {
            if (isTarget(data.ProcessID, data.TimeStamp.ToFileTimeUtc()))
            {
                sink(Capture(data, XamlProvider.KernelImage,
                    [new("ImageBase", data.ImageBase), new("ImageSize", data.ImageSize), new("ImageName", data.FileName)]));
            }
        }
    }

    private static ProfilerEvent Capture(TraceEvent data, XamlProvider provider, EventField[] fields)
    {
#pragma warning disable CS0618 // The raw QPC is shared by the live ETW sessions and saved traces.
        return new(data.TimeStampQPC, data.TimeStampRelativeMSec, data.ProcessID, data.ThreadID,
            provider, false, data.EventName, (int)data.Opcode, fields, Version: data.Version);
#pragma warning restore CS0618
    }
}
