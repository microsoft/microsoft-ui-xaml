// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource2.h"

TableViewSource2::TableViewSource2(winrt::IInspectable const& items)
{
    m_items.set(items);
}

TableViewSource2::~TableViewSource2() = default;

winrt::TableViewSource2 TableViewSource2::From(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }

    return winrt::make<::TableViewSource2>(items);
}

winrt::TableViewSource2 TableViewSource2::Filter(winrt::TableViewPredicate const& /*predicate*/)
{
    ThrowNotImplemented(L"Filter");
}

winrt::TableViewSource2 TableViewSource2::GroupBy(winrt::TableViewKeySelector const& /*key*/, winrt::SortDirection /*groupDirection*/)
{
    ThrowNotImplemented(L"GroupBy");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::hstring const& /*sortMemberPath*/, winrt::SortDirection /*direction*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewKeySelector const& /*key*/, winrt::SortDirection /*direction*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewComparer2 const& /*comparer*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::ParentBy(winrt::TableViewKeySelector const& /*key*/, winrt::TableViewKeySelector const& /*parentKey*/)
{
    ThrowNotImplemented(L"ParentBy");
}

winrt::TableViewSource2 TableViewSource2::ChildrenBy(winrt::TableViewChildrenSelector2 const& /*childrenSelector*/)
{
    ThrowNotImplemented(L"ChildrenBy");
}

winrt::TableViewSource2 TableViewSource2::ClearFilter()
{
    ThrowNotImplemented(L"ClearFilter");
}

winrt::TableViewSource2 TableViewSource2::ClearGroupBy()
{
    ThrowNotImplemented(L"ClearGroupBy");
}

winrt::TableViewSource2 TableViewSource2::ClearSort()
{
    ThrowNotImplemented(L"ClearSort");
}

winrt::TableViewSource2 TableViewSource2::ClearHierarchy()
{
    ThrowNotImplemented(L"ClearHierarchy");
}

void TableViewSource2::ThrowNotImplemented(wchar_t const* verb)
{
    throw winrt::hresult_not_implemented(std::wstring(L"TableViewSource2.").append(verb).append(L" is not implemented yet."));
}
