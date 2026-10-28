// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.App.ViewModels;

// The session file the Frames and Elements tabs show: the connection's recording, or a file the user opened, which stays
// until the next connection's recording.
public sealed class SessionSource
{
    private string? recordingPath;
    private bool following = true;

    public event EventHandler? Changed;

    public string? Path { get; private set; }

    // True while the connection still writes the file.
    public bool Live { get; private set; }

    public void ShowRecording(string? sessionPath, bool connected)
    {
        if (sessionPath is not null && sessionPath != recordingPath)
        {
            recordingPath = sessionPath;
            following = true;
        }

        if (following)
        {
            Set(sessionPath, connected && sessionPath is not null);
        }
    }

    public void Open(string path)
    {
        following = false;
        Set(path, false);
    }

    private void Set(string? path, bool live)
    {
        if (path == Path && live == Live)
        {
            return;
        }

        Path = path;
        Live = live;
        Changed?.Invoke(this, EventArgs.Empty);
    }
}
