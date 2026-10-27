// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Builds one view of the target (counts now; object lifetimes, frames later) from the event stream.
// Called on the processing thread, one event at a time, in arrival order.
public interface IEventProcessor
{
    void Process(ProfilerEvent profilerEvent);
}
