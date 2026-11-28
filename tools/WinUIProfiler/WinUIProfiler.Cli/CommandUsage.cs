// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Cli;

/// <summary>
/// What a command worked on, for its usage event. Commands fill it in only while usage telemetry is on.
/// </summary>
internal sealed class CommandUsage
{
    /// <summary><c>etl</c>, <c>session</c> or <c>live</c>; null when the command read no trace, session file or app.</summary>
    public string? Input { get; set; }

    /// <summary>
    /// Names the app the command worked on, as <see cref="WinUIProfiler.Engine.Target.TelemetryTarget"/> names it, once
    /// the command has ended; null when it worked on no single app.
    /// </summary>
    public Func<string>? App { get; set; }
}
