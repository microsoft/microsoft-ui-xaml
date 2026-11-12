---
name: analyze-session
description: >-
  Opens a WinUI Profiler session file (.sqlite) and answers questions about a
  WinUI 3 app's performance from it with the winui-profiler tools: how the
  launch went, UI-thread ticks and slow ticks, regions of interest
  (navigations, flyouts and dialogs opening, focus changes), and the UI
  elements and composition visuals the events mention. Use it when someone
  asks why a WinUI app is slow to start or draws slowly, what a tick did, what
  happened around a navigation or to an element, or what a session holds, and
  for /winui-profiler:analyze-session [session.sqlite | folder].
user-invocable: true
---

# Analyze a WinUI Profiler session

The WinUI Profiler app (`tools/WinUIProfiler` in the microsoft-ui-xaml
repository) records one WinUI 3 process into a session file. Its processors
store each UI thread's ticks, the regions of interest, and the UI elements
and composition visuals the events mention. The winui-profiler tools read
them, even while the app is still recording, and never change the file.

## Pick the session

`/winui-profiler:analyze-session [target]` opens a session; the target is the
text after the command. A question, with or without the command, is about
the session file it names, or else the newest one.

- **A `.sqlite` file:** call `sessions` with view `summary` and `sessionPath`
  set to it. A file name alone means one in the sessions folder,
  `%LOCALAPPDATA%\WinUIProfiler\Sessions`.
- **Nothing:** call `sessions` with view `summary` and no `sessionPath`. It
  reads the newest file in the sessions folder; say which file that is.
- **A folder:** call `sessions` with view `list` and `folder`. Show the
  sessions (file, process, coverage, start, how it ended, events), ask which
  one to open, and stop.
- **A `.etl` file:** the tools don't read traces. Say how to read it into a
  session file (Getting a session, below), and stop.

If the session can't be read, give the tool's error, offer `sessions` with
view `list`, and stop. Pass the file the summary read as `sessionPath` to
every later call, so that every answer comes from the same file: the newest
file changes when the app starts another recording.

When the command only opened a session, reply briefly from its summary, then
stop and call nothing else until the user asks a question:

- the file, and the process (name and ID);
- the coverage, and how the recording ended, or that it's still recording;
- the time span, and the stored events, ticks, elements and visuals;
- which analyses have data, and why the others don't;
- the notes that limit answers, such as lost or deleted events;
- two or three questions the session can answer, from the analyses it has.

## Answer a question

1. Call `sessions` with view `summary` first, once per session. Its
   `analyses` say which tools and views have data and, when one hasn't, why;
   its `notes` say what limits the answers.
2. Call only the views the question needs, as the table below says. Lists are
   capped by `limit`; a result with `nextOffset` continues from `offset`.
3. Answer only from what the tools return. Give numbers with their units, and
   the tick, region, thread or object they belong to.
4. Say what was read: the file, the process and the time base. Pass on the
   notes that change the answer: still recording, events lost or dropped, the
   oldest events deleted, a session that starts at the connection.
5. End with How I got this.

| Question | Call |
| --- | --- |
| Which sessions are there? | `sessions`, view `list` |
| What does the session hold? How long does it run? | `sessions`, view `summary` (its `uiThreads`) |
| How did the launch go? | `launch`, view `milestones` |
| How soon did the first frames draw? | `launch`, view `frames` |
| Does the app draw slowly? | `frames`, view `summary`, then `slowest` |
| What did one slow tick do? | `frames`, view `tick` with its `tickId` |
| What happened when a page, flyout or dialog opened? | `frames`, view `regions`, then `rows` with its `regionId`, then `tick` for the tick it ends with |
| When did layout passes, navigations or focus changes happen? | `frames`, view `rows` with `type` |
| Which elements and visuals were there? | `objects`, view `types`, then `list` |
| What happened to one element or visual? | `objects`, view `history` |

## How I got this

End every reply, including a session's summary, a list of sessions and an
error, with a section headed **How I got this**, which shows how the answer
came from the session file. The tools don't take SQL: each view runs fixed
statements, with the arguments' values written in, then sorts, groups,
counts and pages their rows in code. Each result says how: `queries` lists
the statements the call ran, each once, and `computed` says what the call
did with their rows.

1. Name the session file, and say that each statement runs on it as is,
   read-only, with `sqlite3 -readonly <file>`. For `sessions` view `list`,
   name the folder: its statement ran once on each file it lists that could
   be read. The first time in a conversation that a statement you show reads
   `InterestingRows`, `InterestingRowEvents` or `FrameList`, add that SQLite
   3.54.0 and 3.54.1 can't run it ("VIEWs and/or subqueries nested too
   deep"), but another version, such as 3.53, can.
2. Then each call the answer used, in the order they were made: the tool, the
   view and the arguments passed. Under it, each of its `queries` in order:
   its `sql`, copied exactly, in a code block marked `sql`; after the block,
   its `rows` (of all its runs), its `runs` when it ran more than once, and
   its `files` when the result gives them. Then the call's `computed`, copied
   exactly, after "Then:".
3. Last, one or two sentences on how those results led to the answer.

Rules:

- Copy each `sql` and `computed` exactly as the tool returned them. Never
  write, shorten, reformat or reconstruct SQL, and never show a statement no
  tool returned. Don't add to a `computed`: what you conclude from the
  results goes in the last step.
- A call whose `queries` is empty ran no SQL; say so. A call that failed
  returns only its error: give the error, and don't guess what it ran.
- Show a statement once per answer. When a later call ran it again, say
  which call showed it. When an answer needed no new call, name the earlier
  calls it came from.
- The statements read the file as it was. A session still being recorded can
  return more rows when one is run again.
- If the user asks not to see the section, leave it out until they ask for it
  again.

## Reading the results

- **Ticks.** A tick drew when XAML's frame counter moved on by the thread's
  next tick, so whether the latest tick drew isn't known. That isn't proof
  that a frame reached the screen: report tick durations and slow ticks, and
  don't call frames dropped. Slow means more than 16.7 ms, a frame at 60 Hz;
  the display's refresh rate isn't recorded.
- **Tick times** are wall-clock time on the UI thread, not CPU time: a long
  tick may have been waiting, and nothing in a session says on what. Events
  on render and decode threads, and on threads that never tick, belong to no
  tick.
- **Types.** An element's type is the nearest built-in type, so an app's own
  control shows as its base class, unless only a template, Loaded or render
  event named it. A render event's name has no namespace, so one type can
  have two rows, such as `TransitionRoot` and
  `Microsoft.UI.Xaml.TransitionRoot` (`type` `TransitionRoot` lists both).
  Objects no event gave a type are marked `typeUnknown`, which isn't a type
  of its own. A freed object's address can be reused: `history` then lists
  the objects that had it, and asks for one's `objectId`.
- **Builds.** Ticks, regions and launch come from Microsoft-Windows-XAML in
  every WinUI build. Current builds write the Microsoft-Windows-XAML-Profiler
  events whenever a session enables the provider. Older builds wrote most of
  them only with `XamlProfilerEnabled`, which WinUI's own build set for Debug
  builds only, so a session of an older Release build has no visuals; its
  elements come from the Microsoft-Windows-XAML events that name them. A
  visual has a name, its owner element's label, only in older builds.
- **Times** are in milliseconds from the origin `session.timeBase` gives: for
  a live session, the start of the recording; for a trace, the process's
  first event in it. It also says when the process was created, when known:
  for a launch recorded from process start, or a trace with the kernel's
  process events, as a WPR trace has. `launch` then gives
  `msFromProcessStart`, unless the session starts at the connection, and
  `measuredFrom` says what its frames count from.
- **Length.** Each of the summary's `uiThreads` says when its first tick
  started (`firstStartMs`) and its last finished tick ended (`lastEndMs`).
  For a live session, `startedUtc` to `endedUtc` is how long the recording
  ran. For a session read from a trace, they're when the trace was read, not
  when it was recorded, which the file doesn't say.
- **Limits.** One process per session. A session file is limited to 500 MB:
  past it, the oldest events are deleted, and the summary says from which
  event the file is complete.

## What the tools can't answer

These aren't stored, so say so, and don't estimate them from other data:

- the events' field values, and counts of raw events by provider;
- GPU memory, CPU samples and call stacks, context switches and thread waits;
- the XAML and composition trees;
- the Launch tab's milestones from the marker configuration, such as an app's
  own markers (a marker event fired on a UI thread shows only among its
  tick's events, in the Other bucket; one fired on another thread doesn't);
- other processes, such as the one that launched the app.

## Getting a session

- **Record it in the WinUI Profiler app.** The connection modes **Unpackaged
  executable** and **Packaged application** launch the app after the session
  starts; **Attach to process ID** starts at the connection, after most of
  the launch. Recording live needs an elevated WinUI Profiler, or the local
  Performance Log Users group. Sessions are saved in the sessions folder,
  named `<date>-<time>-<process>-<pid>.sqlite`.
- **Read a trace (.etl) into one,** which needs no elevation: in the app,
  with the connection mode **Recorded trace (.etl)**, then **Open trace**, or
  with the WinUI Profiler CLI's
  `record <trace.etl> --pid <pid> --out <session.sqlite>`.
