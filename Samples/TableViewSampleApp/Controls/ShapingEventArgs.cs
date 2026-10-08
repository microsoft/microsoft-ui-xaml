// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace TableViewSampleApp.Controls;

/// <summary>Raised by <see cref="ShapingOptions"/> just before it calls GroupBy or ClearGroupBy.</summary>
public sealed class ShapingApplyingEventArgs : EventArgs
{
    public ShapingApplyingEventArgs(string mode, string key)
    {
        Mode = mode;
        Key = key;
    }

    /// <summary>"flat" or "grouped": the mode about to be applied.</summary>
    public string Mode { get; }

    /// <summary>The group-key Tag about to be applied (also set in flat mode).</summary>
    public string Key { get; }
}

/// <summary>
/// Raised by <see cref="ShapingOptions"/> after GroupBy or ClearGroupBy returned, the selection was
/// restored and the gating updated. SamplePageBase writes <see cref="Text"/> to the Shaping readout
/// and, when <see cref="Announce"/> is set, <see cref="Message"/> to Last action.
/// </summary>
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

    /// <summary>The visible label of the group key, e.g. "Department".</summary>
    public string KeyLabel { get; }

    public bool IsGrouped => Mode == "grouped";

    /// <summary>True when the change came from the user (a selector), false for a re-apply.</summary>
    public bool Announce { get; }

    /// <summary>How long the GroupBy / ClearGroupBy call took (through <see cref="ShapingOptions.TimeCall"/> when set).</summary>
    public long ElapsedMilliseconds { get; }

    /// <summary>Shaping readout value: "Flat" or "Grouped by Department".</summary>
    public string Text { get; }

    /// <summary>Last action narration: "Shaping -> Flat" or "Shaping -> Grouped by Department". A page may extend it.</summary>
    public string Message { get; set; }
}

/// <summary>A bulk expansion change made by the Shaping section.</summary>
public enum ShapingAction
{
    ExpandedAll,
    CollapsedAll,
}

/// <summary>
/// Raised by <see cref="ShapingOptions"/> after Expand all or Collapse all ran. SamplePageBase
/// runs the page's OnShapingAction hook, then writes <see cref="Message"/> to Last action.
/// </summary>
public sealed class ShapingActionEventArgs : EventArgs
{
    public ShapingActionEventArgs(ShapingAction action, long elapsedMilliseconds, string message)
    {
        Action = action;
        ElapsedMilliseconds = elapsedMilliseconds;
        Message = message;
    }

    public ShapingAction Action { get; }

    /// <summary>How long ExpandAllGroups / CollapseAllGroups took (through <see cref="ShapingOptions.TimeCall"/> when set).</summary>
    public long ElapsedMilliseconds { get; }

    /// <summary>Last action narration: "Expanded all groups" or "Collapsed all groups". A page may extend it.</summary>
    public string Message { get; set; }
}
