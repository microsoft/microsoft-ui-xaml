# Xaml scheduling

Scheduling is the work of determining when the UI thread runs.

## Scheduling thread

In the past when Xaml rendered for itself, we had a "render" thread that ticked independent animations and rendered into
a swap chain. Xaml hasn't done that in a long long time; now we create a tree of Composition Visuals and let the lifted
compositor do the work of evaluating animations and rendering to a surface.

The "render" thread remains around, though. Its primary job now is to wake the UI thread when a frame is needed, so it
has become a scheduling thread. There are still pieces of code left around from the rendering days that are likely no
longer relevant. These are captured at the end.

The basic algorithm for the scheduling thread is simple - see when the next UI thread frame is needed. If it's needed
immediately, then wake the UI thread (making sure to throttle to the refresh rate). Otherwise, sleep until either the
time of the next scheduled frame or until another request for a frame comes in, whichever is earlier.

## UIThreadScheduler

The `UIThreadScheduler` object is shared between the UI thread and the scheduling thread. Its job is to keep track of
when the next frame is needed. If there's a DispatcherTimer with a 2s interval, then we'll need the next frame in 2
seconds. If the app then updates the tree and dirties it, we'll need the next frame right away. (And after we run that
frame, we'll evaluate how much time is left on the timer and schedule another frame for something like 1.95s.)

## WaitForVBlank

We throttle UI thread frames to the refresh rate of the display - it's generally not useful to render faster than the
display can update. WaitForVBlank is the function that can throttle to the refresh rate of the display. However, it
prevents the display from going into a low-power state, so we'd like to avoid calling it whenever we can. In cases where
frames are far apart anyway, there's no need to wait for a VBlank. It's only when the app requests multiple frames in
the same refresh interval that we need to throttle with WaitForVBlank.

The scheduler determines when frames are too close together by logging the previous time that it woke the UI thread and
subtracting to get the time difference. If the time difference is larger than the refresh interval, we skip throttling.
We get the refresh interval from Composition's GetFrameStatistics API.

Note that some of the scenarios that we're optimizing are poor behaving apps to begin with. If an app has a
DispatcherTimer that runs forever, it'll keep the CPU busy and drain the battery unnecessarily, but if the app also
calls WaitForVBlank then it drains even more battery by keeping the display in a higher power state. The ultimate fix is
for the app to stop its DispatcherTimer (which it can find with Microsoft-Windows-XAML's DispatcherTimer_Tick ETW
event), but in the meantime we'll at least avoid the WaitForVBlank cost where we can.

Also note that this WaitForVBlank optimization is fragile to the UI thread requesting too many frames. When a
DispatcherTimer counts down to 0, we'll run the app handler and request another frame after DispatcherTimer.Interval
seconds. It's important that we don't request more UI thread frames aside from that one, otherwise the scheduling thread
can see multiple separate requests and throttle them.

## Defunct, confusing code

There are still vestiges of the old "render" thread and old scheduling algorithms around. We should remove these as
we're making changes. They add unnecessary complexity and mislead people who are looking at the code to see how it
works.

Old "render" thread
* CompositorScheduler has a "draw lists lock". This is the lock that prevented the UI thread from submitting new work
  while the render thread is processing its existing work.
* CompositorScheduler has a "scheduler command lock". The render thread was aware of when the window became invisible so
  it could stop rendering, and this was the lock that updated this state.

Old scheduling algorithm
* We had the concept of "high-priority ticks". It used to be that producing frames and processing input had different
  priority levels in the message queue, which meant that one could starve the other, and we've seen both situations in
  the wild. we ended up putting input above ticking, and "high-priority ticks" were a mechanism to force a frame in
  situations where we think input is starving rendering. Now input and ticking have the same priority so "high-priority
  ticks" shouldn't be needed anymore.

RefreshAlignedClock is probably not needed. See the banner comment in RefreshAlignedClock.h for the scenarios it covers.

## More optimizations

We should revisit and measure Storyboard scenarios. When there's just a Storyboard running, the UI thread shouldn't have
to tick constantly. The initial frame should have produced composition animations, and once we commit that frame we can
just let the lifted compositor tick the animation without any more work from Xaml.

ReplayPointerUpdate is showing up in traces as requesting a lot of UI thread frames. This feature covers scenarios where
the cursor is static but the UI is animating. After the animation completes we request a frame to update the UI's hover
state. One replay shouldn't be kicking off other replays, and animations where things aren't changing position shouldn't
be requesting ticks either.
