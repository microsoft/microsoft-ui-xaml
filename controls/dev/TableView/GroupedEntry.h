// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// A group header row. A data row IS the app item, so "is a GroupedEntry" and "is a group header"
// are the same statement.
//
// The row axis must stay self-identifying: ElementFactoryGetArgs carries only Data and Parent, no
// index, so the element factory picks TableViewGroupHeader vs TableViewRow from the row value alone.
// That is why a header cannot simply be the view's group object.
//
// GroupItemCount / IsExpanded duplicate what TableViewRowInfo reports for the same index, on purpose:
// during a reshape a realized header can briefly resolve to an index whose metadata says Data, and
// PrepareGroupHeaderElement falls back to these so the header does not render a dead chevron.

// Without a real IID, try_as<GroupedEntry>() is not a type test: the default interface of a
// winrt::implements<..., IInspectable> type is IInspectable, which QIs on EVERY WinRT object. Probe
// this tag first; see TryGetGroupedEntry.
struct __declspec(uuid("2E05CF8D-780E-4FBE-84DD-388D0BA5A48B")) IGroupedEntryTag : ::IUnknown {};

class GroupedEntry : public winrt::implements<GroupedEntry, winrt::IInspectable, IGroupedEntryTag>
{
public:
    GroupedEntry(
        winrt::hstring const& identity,
        winrt::IInspectable const& key,
        int32_t groupItemCount,
        int32_t childCount,
        bool isExpanded,
        int32_t level = 0);

    // The group's stable identity: survives a view that recreates its group objects.
    winrt::hstring const& Identity() const noexcept { return m_identity; }
    // The group's key value (TableViewGroupInfo.Key).
    winrt::IInspectable const& Key() const noexcept { return m_key; }
    // The rows the group holds when expanded.
    int32_t GroupItemCount() const noexcept { return m_groupItemCount; }
    // The group's immediate children: its roots under a tree, else its rows.
    int32_t ChildCount() const noexcept { return m_childCount; }
    bool IsExpanded() const noexcept { return m_isExpanded; }
    // 0 for a top-level group; a subgroup of a multi-level view is one deeper than its parent.
    int32_t Level() const noexcept { return m_level; }

private:
    winrt::hstring m_identity;
    winrt::IInspectable m_key{ nullptr };
    int32_t m_groupItemCount{ 0 };
    int32_t m_childCount{ 0 };
    bool m_isExpanded{ true };
    int32_t m_level{ 0 };
};

// The only safe way to test a row value for GroupedEntry, and therefore for "is a group header".
inline winrt::com_ptr<GroupedEntry> TryGetGroupedEntry(winrt::IInspectable const& value)
{
    return value && value.try_as<IGroupedEntryTag>() ? value.try_as<GroupedEntry>() : nullptr;
}
