This is a pointer to a design doc added here to make it more discoverable until it's converted to MarkDown.

Spec: Xaml Object Lifetime Design

Key words to help with search:
* TrackerPtr, TrackerTargetReference
* Peg, unpeg
* Managed peer
* Reference Tracking
* ReferenceTrackerManager
* UIAffinityReleaseQueue
* Garbage Collection, GC
* IReferenceTracker
* RCW, CCW
* AddPeerReferenceToItem
* ReferenceTrackerWalk
* PegNoRef
* SetExpectedReferenceOnPeer, AddExtraExpectedReferenceOnPeer
* AddPeerReferenceToItem
* ExpectedRefCount

## Release-queue cleanup measurement

TiP measure [64884230](https://microsoft.visualstudio.com/OS/_workitems/edit/64884230)
reports `UIAffinityReleaseQueueCleanupTest`. The first 64 eligible asynchronous
cleanup periods per queue are sampled; this is a bounded early-lifetime sample,
not random sampling of all releases.

At the first cleanup pass, the test snapshots the final-release queue length
after transferring GC-unreachable entries. Success requires processing that
many releases. Later arrivals do not extend the sample. Empty passes and
islands-only cores without an island do not start a test. Synchronous cleanup,
including the budget's synchronous fallback, ignores an unfinished sample as
`cancelled`; a cleanup HRESULT fails it.

Metrics include the compile-time WinUI version, queue-to-completion delay, and
processed-release count. Delay buckets are below 16 ms, 16-99 ms, 100-999 ms, and
at least 1 second. Count buckets are 0-1, 2-16, 17-256, and above 256. The delay
starts when the sampled queue's nonempty period began, includes cleanup work
and time spent suspended, and does not itself fail the test. Use this custom
delay metric, not TiP's built-in duration, which starts at the first cleanup
pass. Counts are processed references, not destroyed objects or bytes freed.

Work never serviced by cleanup produces no sample. Normal shutdown ignores
in-flight samples, but TiP can independently report process termination or
expiration; those results alone do not prove a cleanup defect. Compare matching
instrumented workloads and versions, not old local runs against new production
traffic. Public builds may use the no-op SDK; internal pipeline builds require
the production SDK unless explicitly opted out.
