// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A link from an event to an object it mentions, such as "ChildId: Grid "LayoutRoot"", with a badge for its kind.
public sealed class ObjectLink(ObjectMention mention)
{
    public ObjectMention Mention => mention;
    public bool IsElement => mention.Kind == ObjectKind.Element;
    public bool IsVisual => mention.Kind == ObjectKind.Visual;
    public string KindText { get; } = ObjectRow.KindName(mention.Kind);
    public string Text { get; } = $"{mention.Field}: {ObjectRow.Describe(mention.Kind, mention.Type, mention.Name)}";
    // The link's name for screen readers, which don't see the badge.
    public string AccessibleName => $"{KindText}, {Text}";
}
