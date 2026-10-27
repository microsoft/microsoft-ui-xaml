// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Builds one view of the target (counts, the session file; object lifetimes and frames later) from the event stream.
// Called on the processing thread, one event at a time, in arrival order. The sequence number (1, 2, 3...) is the
// event's arrival order and its Seq in the session file, so views can refer to stored events.
public interface IEventProcessor
{
    void Process(long sequence, ProfilerEvent profilerEvent);
}
