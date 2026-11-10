---
name: winui-profiler-analysis
description: >-
  Answers questions about a WinUI 3 app's performance from a WinUI Profiler
  session file (.sqlite) or a recorded ETW trace (.etl), with the
  winui-profiler tools, which read what the WinUI Profiler app's processors
  stored: how the launch went, UI-thread ticks and slow ticks, regions of
  interest (navigations, flyouts and dialogs opening, focus changes), and the
  UI elements and composition visuals the events mention. Use it when someone
  asks why a WinUI app is slow to start or draws slowly, what a tick did, what
  happened around a navigation, what happened to an element, or what a session
  or trace holds.
---

# WinUI Profiler sessions

The WinUI Profiler app (`tools/WinUIProfiler` in the microsoft-ui-xaml
repository) records one WinUI 3 process into a session file. Its processors
build tables from the process's XAML events: the ticks of each UI thread, the
regions of interest and interesting rows, and the UI elements and composition
visuals the events mention. The winui-profiler tools read those tables and
the session file's views over them, and never return the events' field
values. They read the files where they are, without changing them, even while
the app is still recording one. `record_trace` reads a recorded trace into a
new session file first.

## The tools

Each tool has views; `view` picks one, and the first is the default.

| Tool | Views |
| --- | --- |
| `sessions` | `list`: the session files; `summary`: what one holds |
| `record_trace` | Reads a recorded trace (.etl) into a session file |
| `launch` | `milestones`: how the launch went; `frames`: the first frames drawn |
| `frames` | `summary`, `slowest`, `regions`, `rows`, `tick` |
| `objects` | `types`, `list`, `history` |

## Which call answers what

| Question | Call |
| --- | --- |
| Which sessions are there? | `sessions`, view `list` |
| What does this session or trace hold? | `sessions`, view `summary` |
| How long does the session run? | `sessions`, view `summary`: `uiThreads` |
| How did the launch go? | `launch`, view `milestones` |
| How soon did the first frames draw? | `launch`, view `frames` |
| Does the app draw slowly? | `frames`, view `summary`, then `slowest` |
| What did one slow tick do? | `frames`, view `tick` with its `tickId` |
| What happened when a page, flyout or dialog opened? | `frames`, view `regions`, then `rows` with its `regionId` |
| When did layout passes, navigations or focus changes happen? | `frames`, view `rows` with `type` |
| Which elements and visuals were there? | `objects`, view `types`, then `list` |
| What happened to one element or visual? | `objects`, view `history` |

Ticks, regions and launch are described in [frames.md](frames.md). Read it
before answering a question about them.

## What the tools can't answer

The tools read only what the app's processors store. These aren't stored, so
say so and don't estimate them from other data:

- the events' field values, and counts of raw events by provider;
- GPU memory, CPU samples and call stacks, context switches and thread waits;
- the XAML and composition trees;
- the Launch tab's milestones from the marker configuration, such as an app's
  own markers (a marker event fired on a UI thread shows only among the
  events of the tick it fired in, in the Other bucket, under its provider's
  name; one fired on another thread doesn't show);
- other processes, such as the one that launched the app.

More answers come as the app gets processors for them.

## Getting a session

- **Record it in the WinUI Profiler app.** The connection modes **Unpackaged
  executable** and **Packaged application** launch the app, and the session
  starts before the process does. **Attach to process ID** starts at the
  connection, after most of the launch. Recording live needs an elevated
  WinUI Profiler, or membership in the local Performance Log Users group.
  Sessions are saved in `%LOCALAPPDATA%\WinUIProfiler\Sessions`, named
  `<date>-<time>-<process>-<pid>.sqlite`.
- **Read a recorded trace (.etl).** Call `record_trace` with the trace's full
  path, or open the trace in the app (connection mode **Recorded trace
  (.etl)**, then **Open trace**), or use the WinUI Profiler CLI's `record`
  command. Reading a trace needs no elevation. `record_trace` saves the session
  file in the sessions folder unless it's given another `folder`, as the app
  does; the CLI's `record` writes it where its `--out` option says.

### Recording a trace

Start the trace before the app, with every XAML provider at level 5, as in
the WinUI Profiler README:

```powershell
'{531A35AB-63CE-4BCF-AA98-F88C7A89E455} 0xFFFFFFFFFFFFFFFF 5',
'{59E7A714-73A4-4147-B47E-0957048C75C4} 0xFFFFFFFFFFFFFFFF 5',
'{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D} 0xFFFFFFFFFFFFFFFF 5',
'{2DC72F6E-E4D1-5F58-3245-09A4243799DD} 0xFFFFFFFFFFFFFFFF 5',
'{21E0AE07-56A7-55B5-12F9-011E6BC08CCA} 0xFFFFFFFFFFFFFFFF 5',
'{802EC45A-1E99-4B83-9920-87C98277BA9D} 0x4 5',
'{8C416C79-D49B-4F01-A467-E56D3AA8234C} 0x2000 4' | Set-Content providers.txt
logman create trace XamlTrace -o xaml.etl -pf providers.txt -bs 1024 -nb 16 256 -ets
# ... use the app ...
logman stop XamlTrace -ets
```

A trace that doesn't enable Microsoft-Windows-XAML at level 5 has no ticks,
and one without Microsoft-Windows-Win32k (the last line) has no window focus
changes, so no regions start at one.
Recording a trace needs elevation or the Performance Log Users group, as
recording live does.

## How to answer

1. **Pick the session.**
   - A `.sqlite` file: pass it as `sessionPath` to every tool. A file name
     alone means a file in the sessions folder.
   - A `.etl` file: call `record_trace` with its full path. When the trace has
     several XAML processes, the tool asks which one, or returns
     `needsProcessId` with the list; then ask the user and call it again with
     `processId`. Use the returned `session.sessionPath` from then on.
   - Nothing named: the tools read the newest session file. Say which file
     that is; `sessions` with view `list` shows the others.
2. **Call `sessions` with view `summary` first.** Its `analyses` say which
   tools and views have data and, when one hasn't, why. Its `notes` say what
   limits the answers.
3. **Pass the same `sessionPath` to every call,** so that every answer comes
   from the same file: the newest file changes when the app starts another
   recording.
4. **Call only the views the question needs.** Lists are capped by `limit`; a
   result with `nextOffset` continues from `offset`.
5. **Say what was read:** the file, the process and the time base, with the
   notes that limit the answer.
6. **End with How I got this,** as the next section says.

## How I got this

End every answer with a section headed **How I got this**, which shows how
the answer came from the session file. The tools don't take SQL: each view
runs fixed statements, with the arguments' values written in, then sorts,
groups, counts and pages their rows in code. Each result says how: `queries`
lists the statements the call ran, each once, and `computed` says what the
call did with their rows.

1. Name the session file, and say that each statement runs on it as is,
   read-only, with `sqlite3 -readonly <file>`. For `sessions` view `list`,
   name the folder: its statement ran once on each file it lists that could
   be read. The first time in a conversation that a statement you show reads
   `InterestingRows`, `InterestingRowEvents` or `FrameList`, add that SQLite
   3.54.0 and 3.54.1 can't run it ("VIEWs and/or subqueries nested too
   deep"), but another version, such as 3.53, can.
2. Then each call the answer used, in the order they were made: the tool, the
   view and the arguments passed. Under it, each of its `queries` in order:
   - its `sql`, copied exactly, in a code block marked `sql`;
   - after the block, its `rows` (of all its runs), its `runs` when it ran
     more than once, and its `files` when the result gives them.

   Then the call's `computed`, copied exactly, after "Then:".
3. Last, one or two sentences on how those results led to the answer.

Rules:

- Copy each `sql` and `computed` exactly as the tool returned them. Never
  write, shorten, reformat or reconstruct SQL, and never show a statement no
  tool returned. Don't add to a `computed`: what you conclude from the
  results goes in the last step.
- A call whose `queries` is empty ran no SQL; say so. A call that failed
  returns only its error: give the error, and don't guess what it ran.
- Show a statement once per answer. When a later call ran it again, say
  which call showed it.
- When an answer needed no new call, name the earlier calls it came from.
- The statements read the file as it was. A session still being recorded can
  return more rows when one is run again.
- If the user asks not to see the section, leave it out until they ask for it
  again.

## Objects

- `objects` lists the elements and visuals the events mention. View `types`
  counts them by type; view `list` gives the objects of one `type` (full or
  short name, such as `Button`) or whose `name` contains some text, with
  `orderBy` `time` or `events`; view `history` gives one object's events, by
  `objectId` or by `address` (hex or decimal).
- An element's type is the nearest built-in XAML type `DependencyObject_Enter`
  gave, so an app's own control shows as its base class. An element that event
  didn't type gets the class a template, Loaded or render event named: maybe
  the app's own class, or a name without its namespace, so one type can have
  two rows, such as `TransitionRoot` and `Microsoft.UI.Xaml.TransitionRoot`
  (`type` `TransitionRoot` lists both). A visual has a name, its owner
  element's label, only in sessions of older WinUI builds. An object lives
  from its first to its last mention. A freed object's address can be reused;
  when two objects had an address, `history` lists them, a page at a time, and
  asks for one's `objectId`.
- An object that no event gave a type has none. Its row, in view `types` and
  in the `frames` tick view, has `typeUnknown`, and a note counts such objects.
  Don't call it a type of its own.

## Evidence rules

- Answer only from what the tools return. Give numbers with their units, and
  the tick, region, thread or object they belong to.
- Tick times are wall-clock time on the UI thread, not CPU time; nothing in a
  session says what a thread waited on.
- A tick drew when XAML's frame counter moved on by the thread's next tick;
  that isn't proof that a frame reached the screen.
- Pass on the notes that change the answer: still recording, events lost or
  dropped, the oldest events deleted, a session that starts at the connection.

## Times

- Times are in milliseconds, from the origin `session.timeBase` gives: for a
  live session, the start of the recording; for a trace, the first of the
  process's events in the trace. It also says when the process was created,
  relative to that origin, when the session knows: a trace says so only when
  it has the kernel's process events, as a WPR trace has.
- For a session read from a trace, `startedUtc` and `endedUtc` are when the
  trace was read into the session file, not when it was recorded, and the time
  between them is how long the reading took. The file doesn't say when the
  trace was recorded.
- How long a session runs: each of the summary's `uiThreads` says when its
  first tick started (`firstStartMs`) and when its last finished tick ended
  (`lastEndMs`). For a session recorded live, the time from `startedUtc` to
  `endedUtc` is how long the recording ran. A trace's own length isn't stored.
- `launch` also gives `msFromProcessStart` when the session knows when the
  process was created, unless it starts at the connection; `measuredFrom` says
  what its frames count from.

## Builds

- Ticks, regions and launch come from Microsoft-Windows-XAML, whatever the
  WinUI build.
- Current WinUI builds write the Microsoft-Windows-XAML-Profiler events,
  including every composition visual event, whenever a session enables the
  provider. Older builds wrote most of them only with the profiler compiled in
  (`XamlProfilerEnabled`, which WinUI's own build set for Debug builds only),
  so a session of an older Release build has no visuals; its elements come
  from the Microsoft-Windows-XAML events that name them.

## Limits

- One process per session.
- A session file is limited to 500 MB: past it, the oldest events are deleted,
  and the summary says from which event the file is complete.
- Events on render and decode threads, and on threads that never tick, belong
  to no tick.
- A slow tick took more than 16.7 ms, a frame at 60 Hz; the display's refresh
  rate isn't recorded.
