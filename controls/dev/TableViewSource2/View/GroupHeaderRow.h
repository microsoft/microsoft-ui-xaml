// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <winrt/Windows.Foundation.h>

// A group header's row value in ShapedCollectionView (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "WinRT visible list" (V2).
//
// The v2 counterpart of v1 GroupedEntry. A data row's value is the app item; a header's value is
// this token, so the row value identifies its own kind (ElementFactoryGetArgs carries only Data,
// no index) and TableView picks TableViewGroupHeader vs TableViewRow from it.
//
// Holds immutable data only: the group key and GroupNode::Identity. Live state (ItemCount,
// expanded, expandable, level) is read through row metadata by index, never copied, so it cannot go
// stale (v1 GroupedEntry duplicated GroupItemCount / IsExpanded).
//
// Created on the first ShapedCollectionView::GetAtCore for the group and cached on
// GroupNode::HeaderRow, so the header's DataContext is the same object for the group's lifetime.
// A token that outlives its group (held by a recycled element) is detached from the tree but still
// answers Key and Identity. Holds Key strongly, as v1 GroupedEntry does.

// Real IID for GroupHeaderRow. Without it try_as<GroupHeaderRow>() is not a type test: the default
// interface of a winrt::implements<..., IInspectable> type is IInspectable, which every WinRT object
// answers. Probe this tag first; see TryGetGroupHeaderRow.
struct __declspec(uuid("21543EE1-9AA3-474F-B8C0-B506626354E2")) IGroupHeaderRowTag : ::IUnknown {};

class GroupHeaderRow : public winrt::implements<GroupHeaderRow, winrt::Windows::Foundation::IInspectable, IGroupHeaderRowTag>
{
public:
    GroupHeaderRow(winrt::Windows::Foundation::IInspectable const& key, winrt::hstring const& identity);

    // The GroupBy selector's result for this group (GroupNode::Key). May be null (a null key).
    winrt::Windows::Foundation::IInspectable const& Key() const noexcept { return m_key; }

    // GroupNode::Identity: "g|"-prefixed, also the header's IKeyIndexMapping key (V3).
    winrt::hstring const& Identity() const noexcept { return m_identity; }

private:
    winrt::Windows::Foundation::IInspectable m_key{ nullptr };
    winrt::hstring m_identity;
};

// The only safe way to test a row value for GroupHeaderRow, and therefore the test for "is this row
// a group header". See IGroupHeaderRowTag.
inline winrt::com_ptr<GroupHeaderRow> TryGetGroupHeaderRow(winrt::Windows::Foundation::IInspectable const& value)
{
    return value && value.try_as<IGroupHeaderRowTag>() ? value.try_as<GroupHeaderRow>() : nullptr;
}
