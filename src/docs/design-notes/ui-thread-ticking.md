# UI Thread Ticking

## Table of Contents

- [Background](#background)
- [Posting a tick](#posting-a-tick)
- [High and low priorities](#high-and-low-priorities)

## Background

Ticks handle layout, animations, rendering, and the like on the UI thread. Whenever something in the tree changes that 
requires one of these things to happen, we request a tick. At that time, we'll put some message in the message queue 
with the intention of ticking when that message comes through. 

There are two mechanisms that handle ticking:
1. Posting a tick
2. High and low priorities

## Posting a tick

The first time that something requests a tick, we'll post a `WM_INTERNAL_TICK` message to ourselves. To provide fairness 
with input, ticks are queued via a `DispatcherQueue` timer, which runs at the same priority as input. This allows a 
direct FIFO queue where any already-queued input gets processed before the requested tick is processed, and any new 
input which comes in after the tick is requested will be processed after the tick.

## High and low priorities

Because in the past input and tick weren't in the same queue, there was a concept of low priority and high priority 
ticks which were used to help create fairness. These two priorities still exist but no longer impact when ticks get 
processed; the `ListView` code still checks for high priority ticks to minimize how much low-priority background 
processing it does when a high priority tick is pending.

Most changes cause high priority ticks, with the one exception being ticks that are requested while we're in the middle 
of ticking. An example of a low priority tick would be a dependent `DoubleAnimation` requesting that it be evaluated 
again in 16ms. An example of a high priority tick would be app code attaching a new element to the tree in response to 
a button being clicked. It's possible that multiple ticks are requested before the next frame is generated. In that 
case, we could post a `WM_INTERNAL_TICK` while in low-priority mode, then find out that we're in high-priority mode when 
that message arrives.