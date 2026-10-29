// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A link from an event to a UI element it mentions, such as "ChildId: Grid "LayoutRoot"".
public sealed class ElementLink(ElementMention mention)
{
    public long ObjectId => mention.ObjectId;
    public string Text { get; } = $"{mention.Field}: {ElementRow.Describe(mention.Type, mention.Name)}";
}
