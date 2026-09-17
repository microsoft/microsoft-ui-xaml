// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Marks a group header row and carries what the header needs that an index alone cannot supply.
//
// A data row IS the app item, so this is produced for headers only — "is a GroupedEntry" and "is a
// group header" are the same statement, which is why there is no IsGroupHeader() to ask.
//
// The row axis must stay self-identifying: ElementFactoryGetArgs carries only Data and Parent, no
// index (microsoft.ui.xaml.coretypes.idl:1730), so the element factory picks TableViewGroupHeader
// vs TableViewRow from the row value alone. That is the reason a header cannot simply be the app's
// group object.
//
// GroupItemCount / IsExpanded duplicate what TableViewRowInfo reports for the same index, and the
// duplication is deliberate: during a reshape a realized header can briefly resolve to an index
// whose metadata says Data, and PrepareGroupHeaderElement falls back to these so the header does
// not render a dead chevron in that window.

// Real IID for GroupedEntry. Without it try_as<GroupedEntry>() is not a type test: the default
// interface of a winrt::implements<..., IInspectable> type is IInspectable, which QIs successfully
// on EVERY WinRT object, so the com_ptr comes back non-null for an app item and dereferences
// garbage. Probe this tag first; see TryGetGroupedEntry below.
struct __declspec(uuid("2E05CF8D-780E-4FBE-84DD-388D0BA5A48B")) IGroupedEntryTag : ::IUnknown {};

class GroupedEntry : public winrt::implements<GroupedEntry, winrt::IInspectable, IGroupedEntryTag>
{
public:
    GroupedEntry(
        winrt::IInspectable const& group,
        int32_t groupItemCount,
        bool isExpanded);

    winrt::IInspectable Group() const;
    int32_t GroupItemCount() const;
    bool IsExpanded() const;

private:
    winrt::IInspectable m_group{ nullptr };
    int32_t m_groupItemCount{ 0 };
    bool m_isExpanded{ true };
};

// The only safe way to test a row value for GroupedEntry, and therefore the test for "is this row a
// group header". See IGroupedEntryTag.
inline winrt::com_ptr<GroupedEntry> TryGetGroupedEntry(winrt::IInspectable const& value)
{
    return value && value.try_as<IGroupedEntryTag>() ? value.try_as<GroupedEntry>() : nullptr;
}
