---
name: open-session
description: >-
  Opens a WinUI Profiler session for questions, for the command
  /winui-profiler:open-session [session.sqlite | folder | trace.etl]: reads a
  trace into a session file if needed, summarizes the session, and stops. Use
  it only when that command is run; to answer questions about a session, use
  winui-profiler-analysis.
user-invocable: true
---

# Open a WinUI Profiler session

`/winui-profiler:open-session [target]`. The target is the text after the
command: a session file (`.sqlite`), a folder of them, a recorded trace
(`.etl`), or nothing for the newest session.

## Steps

1. **No target:** call `sessions` with view `summary` and no `sessionPath`. It
   reads the newest file in `%LOCALAPPDATA%\WinUIProfiler\Sessions`.
2. **A trace (`.etl`):** make the path full (relative to the current folder),
   and call `record_trace` with it.
   - When the trace has several XAML processes, the tool asks which one, or
     returns `needsProcessId` with the list. Then show the processes (ID,
     name, events, first and last time), ask the user which one, and call
     `record_trace` again with `processId`.
   - It reuses a session it already read from the same trace and process.
     Pass `reread: true` only if the user asks to read the trace again.
   - Then call `sessions` with view `summary` and the returned
     `session.sessionPath`.
3. **A folder:** call `sessions` with view `list` and `folder`, show the
   sessions (file, process, coverage, start, how it ended, events), ask which
   one to open, and stop.
4. **Anything else:** call `sessions` with view `summary` and `sessionPath`
   set to it.

If a tool returns an error, give its message, offer `sessions` with view
`list`, and stop.

## Reply

Keep it short:

- the file, and the process (name and ID);
- the coverage, and how the recording ended, or that it's still recording;
- the time span, and the stored events, ticks, elements and visuals;
- which analyses have data, and why the others don't;
- the notes that limit answers, such as lost or deleted events;
- two or three questions the session can answer, picked from the analyses it
  has.

Then stop. Don't call other tools until the user asks a question. Answer
questions with the winui-profiler-analysis skill, passing this session's
`sessionPath` to every tool.
