// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace TableViewSampleApp.Controls;

public sealed class ShapingApplyingEventArgs : EventArgs
{
    public ShapingApplyingEventArgs(string mode, string key)
    {
        Mode = mode;
        Key = key;
    }

    public string Mode { get; }

    public string Key { get; }
}

public sealed class ShapingAppliedEventArgs : EventArgs
{
    public ShapingAppliedEventArgs(string mode, string key, string keyLabel, bool announce, long elapsedMilliseconds, string text, string message)
    {
        Mode = mode;
        Key = key;
        KeyLabel = keyLabel;
        Announce = announce;
        ElapsedMilliseconds = elapsedMilliseconds;
        Text = text;
        Message = message;
    }

    public string Mode { get; }

    public string Key { get; }

    public string KeyLabel { get; }

    public bool IsGrouped => Mode == "grouped";

    public bool Announce { get; }

    public long ElapsedMilliseconds { get; }

    public string Text { get; }

    public string Message { get; set; }
}

public enum ShapingAction
{
    ExpandedAll,
    CollapsedAll,
}

public sealed class ShapingActionEventArgs : EventArgs
{
    public ShapingActionEventArgs(ShapingAction action, long elapsedMilliseconds, string message)
    {
        Action = action;
        ElapsedMilliseconds = elapsedMilliseconds;
        Message = message;
    }

    public ShapingAction Action { get; }

    public long ElapsedMilliseconds { get; }

    public string Message { get; set; }
}
