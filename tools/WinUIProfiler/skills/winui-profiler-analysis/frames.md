# Ticks, regions and launch

How the `frames` and `launch` tools see a UI thread's work, what each view
returns, and which to use for common questions. They read the tables the
WinUI Profiler app's frame processor builds, and the session file's `Ticks`,
`InterestingRows` and `LaunchSummary` views over them, as its Frames tab does.

## The model

- XAML writes a `CoreServices_Frame` event at the start and at the end of each
  tick of a UI thread, when Microsoft-Windows-XAML is enabled at verbose level.
  Each one carries XAML's frame counter.
- **A tick drew** when the thread's next tick has a higher frame number. The
  thread's latest tick has no next tick, so whether it drew isn't known.
- **A slow tick** took more than 16.7 ms, a frame at 60 Hz.
- **Events belong to ticks:** each event in a tick is in a bucket: Xaml,
  Composition or Other. Events on render and decode threads, and on threads
  that never tick, belong to no tick.
- **A region of interest** starts at a navigation, a PerfXamlEvent operation
  (such as WinUI initializing, a flyout or a ContentDialog opening) or a
  window focus change, and ends when that thread's next tick ends.
- **Interesting rows** are the ticks and UpdateLayout passes inside regions,
  and the operations, navigations, focus changes and graphics device
  creations anywhere.
- **Times** are wall-clock time on the UI thread, not CPU time: a long tick
  may have been waiting rather than working.

## `frames`

### `summary` (the default)

Per UI thread: ticks, how many drew, didn't draw or aren't known to have
drawn, how many were slow, tick durations (count, total, p50, p90, max), the
drawing ticks' durations, events by bucket, and when the first tick started
and the last one ended. Also the regions by trigger, the interesting rows by
type, and when the first frame drew.

### `slowest`

The longest ticks (`orderBy` `duration`, the default) or those with the most
events (`events`), each with its `tickId`, frame number, whether it drew, its
duration, events by bucket, and the region that ends with it. By duration,
ticks without a known duration aren't ranked; a note says how many. `limit`
defaults to 20.

### `regions`

The regions of interest, longest first (`orderBy` `duration`) or in order
(`time`), each with its `regionId`, trigger, thread, start, stop, duration,
the tick it ends with and its rows by type; and the rows outside every region.
`limit` defaults to 20.

### `rows`

The interesting rows in order (`orderBy` `time`) or longest first
(`duration`), each with its type, name, thread, start, stop, duration, frame
number, `tickId` and `regionId`. `regionId` keeps one region's rows, and
`type` one type: Frame, UpdateLayout, Operation, FocusChange,
CreateGraphicsDevice, Navigating or Navigated. `limit` defaults to 50.

### `tick`

One tick, by `tickId`: its thread, frame number, whether it drew, start, end,
duration and events by bucket; the region it ends; its events counted by
provider and name, for the 30 names with the most events; the elements and
visuals they mention, counted by kind and type, for the 20 types with the
most objects; and a page of its events in order, each with its bucket and
the objects it mentions. `limit` defaults to 20. `eventNames` and
`objectTypes` say how many names and types there are, and a note counts any
left out. Objects that no event gave a type are counted in one row per kind,
marked `typeUnknown`, with a note. `bucket` keeps one bucket's events, names
and types; `total` still counts all the tick's events, and `matching` the
bucket's. Events' field values aren't shown.

## `launch`

### `milestones` (the default)

In time order: when the process was created (for a session recorded from
process start), when the recording started (not for a trace), the first tick,
the first of each PerfXamlEvent operation (such as WinUI initializing on a
thread) with its duration and count, graphics device creation, the first
UpdateLayout pass in a region, the first navigation and focus change, the
first region and the first frame drawn. Each has `ms`, and, for a session
recorded from process start, `msFromProcessStart` when the process's creation
is known.

### `frames`

The first ten ticks that drew on the first UI thread: when each ended, from
the process's creation for a session recorded from process start, else from
the start of the recording (`measuredFrom` says which); the time since the
previous one; and each tick's own start, end and duration.

A session that starts at the connection misses most of the launch, and a
trace doesn't say when the process was created; the notes say so.

## Recipes

### Does the app draw slowly?

1. `frames`, view `summary`: each UI thread's tick p90 and maximum, and how
   many ticks were slow.
2. `frames`, view `slowest`: the worst ticks, and the regions they end.
3. For one of them, `frames`, view `tick` with its `tickId`: its events by
   provider and name, and the elements they mention.

Report tick durations and how many were slow. Don't call frames dropped: a
tick's state isn't proof of what reached the screen.

### What happened when a page, flyout or dialog opened?

1. `frames`, view `regions`: the regions by trigger, longest first.
2. `frames`, view `rows` with the region's `regionId`: its ticks and
   UpdateLayout passes.
3. `frames`, view `tick` for the tick the region ends with.

### How did the launch go?

1. `launch`, view `milestones`: process creation, WinUI initializing, the
   first navigation and the first frame drawn.
2. `launch`, view `frames`: how soon the first ten frames drew, and how long
   each drawing tick took.

Only a session recorded from process start measures from the process's
creation.
