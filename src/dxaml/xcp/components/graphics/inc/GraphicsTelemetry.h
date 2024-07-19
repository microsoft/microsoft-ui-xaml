// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// GUID for "Microsoft-Windows-Xaml": {531a35ab-63ce-4bcf-aa98-f88c7a89e455}
DECLARE_TRACELOGGING_CLASS(GraphicsTelemetryLogging, "Microsoft-Windows-XAML", (0x531a35ab, 0x63ce, 0x4bcf, 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55));

class GraphicsTelemetry final : public TelemetryBase
{
    IMPLEMENT_TELEMETRY_CLASS(GraphicsTelemetry, GraphicsTelemetryLogging);

public:

    // Enqueuing a task on Xaml's DeferredInvoke queue. The first time something enters this queue, we'll start the
    // Composition DispatcherQueueTimer that calls us back to process items in this queue - see
    // CXcpDispatcher::QueueDeferredInvoke.
    DEFINE_TRACELOGGING_EVENT_PARAM1(DeferredInvoke_Enqueue,
        uint32_t, WorkCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Dequeuing a task from Xaml's DeferredInvoke queue. This happens before the task starts executing - see
    // CDeferredInvoke::DispatchQueuedMessage.
    DEFINE_TRACELOGGING_EVENT_PARAM1(DeferredInvoke_Dequeue,
        uint32_t, WorkCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Starting the Composition DispatcherQueueTimer as a result of enqueuing the first work item in the DeferredInvoke
    // queue. This timer runs at the same priority as input, and is the mechanism that ensures rendering and input do
    // not starve each other.
    DEFINE_TRACELOGGING_EVENT(DispatcherQueueTimer_Start,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // The callback for the DispatcherQueueTimer that causes Xaml to do work. This will pop a single item off of the
    // DeferredInvoke queue...
    DEFINE_TRACELOGGING_EVENT(DispatcherQueueTimer_Callback,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // ...and if the DeferredInvoke queue has more items in it, then the callback method automatically restarts the
    // DispatcherQueueTimer so we get called back again to process the queue. Note that processing a work item may
    // enqueue more work into the DeferredInvoke queue. We'll just keep requesting timer ticks until we get through them
    // all.
    DEFINE_TRACELOGGING_EVENT(DispatcherQueueTimer_Callback_Restart,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    //
    // The imaging pipeline makes use of the DeferredInvoke mechanism, but with an extra layer of indirection. The
    // ImageTaskDispatcher has a separate queue of imaging-related work to be done on the UI thread, and it puts an
    // entry in the DeferredInvoke queue to do that work. However, unlike the DeferredInvoke queue, the
    // ImageTaskDispatcher is not restricted to processing a single item per callback. Instead, it will count up all the
    // work it currently has and process them all. This batching approach makes sense when the app has a large number of
    // images. It's wildly inefficient to request a UI thread callback for every single download and decode of every
    // single image.
    //
    // As part of processing that work we may schedule more imaging work (e.g. when the download completes we schedule a
    // decode), and we'll put another item in the DeferredInvoke queue to come back in a later tick to take care of the
    // new work. This prevents imaging work infinitely looping and hanging the app, which can happen if an imaging task
    // hits a failure and queues another decode in an attempt to recover. CSvgImageSource::OnDownloadImageAvailableImpl
    // does this in response to device lost errors, for example.
    //
    // Due to the multiple layers of indirection involved, an imaging work item may be scheduled in many different ways:
    //
    //  1. If there's already something in the ImageTaskDispatcher and it already requested a DeferredInvoke, then the
    //     new work just gets queued up in the ImageTaskDispatcher. It'll get processed when the DeferredInvoke comes
    //     in.
    //
    //  2. If there's nothing in the ImageTaskDispatcher, then it hasn't requested anything via a DeferredInvoke. The
    //     new work gets queued, then we put an entry in the DeferredInvoke queue. However, if the DeferredInvoke queue
    //     is nonempty, then we stop there. We already have a DispatcherQueueTimer going that'll call back to process a
    //     DeferredInvoke, and we need to wait until all existing DeferredInvokes are completed before we can get to the
    //     new ImageTaskDispatcher entry for this new imaging work.
    //
    //  3. If there's nothing in the ImageTaskDispatcher, _and_ there's nothing in the DeferredInvoke queue, then the
    //     new work gets queued in ImageTaskDispatcher, which will request a DeferredInvoke, which will also start the
    //     DispatcherQueueTimer. When that timer fires, we'll get to the first DeferredInvoke item, which will pump the
    //     ImageTaskDispatcher and get to the new imaging work item.
    //
    // It's only in case 3 that an imaging task directly requests a tick. Most of the time imaging work just gets queued
    // up and waits for an existing tick to come in.
    //
    DEFINE_TRACELOGGING_EVENT_PARAM2(ImageTaskDispatcher_QueueTask,
        bool, WorkQueued,
        uint32_t, taskCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // When the DeferedInvoke calls back to the ImageTaskDispatcher and we start executing imaging work. The param
    // indicates how many imaging work items we're going to execute.
    DEFINE_TRACELOGGING_EVENT_PARAM1(ImageTaskDispatcher_Execute,
        uint32_t, taskCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // When the DeferedInvoke calls back to the ImageTaskDispatcher and we finish executing imaging work. The
    // "taskCount" param indicates how many new imaging work items were queued up as a result of executing the ones that
    // we already had. They'll be handled in a separate tick. The "WorkQueued" param indicates whether we put something
    // in a DeferredInvoke queue, which happens iff "taskCount" is nonzero.
    DEFINE_TRACELOGGING_EVENT_PARAM2(ImageTaskDispatcher_Execute_End,
        bool, WorkQueued,
        uint32_t, taskCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
};
