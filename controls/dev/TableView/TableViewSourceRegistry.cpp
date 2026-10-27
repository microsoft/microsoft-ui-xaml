// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSourceRegistry.h"
#include "TableViewSource.h"
#include "TableViewSource2.h"

#include <array>
#include <string_view>

namespace
{
    struct SourceVariant
    {
        std::wstring_view Name;
        winrt::ITableViewShapingSource (*Create)(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector);
    };

    // The first entry is the default. Add a variant here.
    constexpr std::array s_variants
    {
        SourceVariant{ L"default", [](winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector) -> winrt::ITableViewShapingSource
            {
                return ::TableViewSource::From(items, keySelector);
            } },
        SourceVariant{ L"v2", [](winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector) -> winrt::ITableViewShapingSource
            {
                return ::TableViewSource2::From(items, keySelector);
            } },
    };
}

winrt::ITableViewShapingSource TableViewSourceRegistry::Create(winrt::hstring const& variant, winrt::IInspectable const& items)
{
    return Create(variant, items, nullptr);
}

winrt::ITableViewShapingSource TableViewSourceRegistry::Create(winrt::hstring const& variant, winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector)
{
    for (auto const& entry : s_variants)
    {
        if (entry.Name == std::wstring_view{ variant })
        {
            return entry.Create(items, keySelector);
        }
    }
    throw winrt::hresult_invalid_argument(L"Unknown TableView source variant.");
}

winrt::IVectorView<winrt::hstring> TableViewSourceRegistry::Variants()
{
    std::vector<winrt::hstring> names;
    names.reserve(s_variants.size());
    for (auto const& entry : s_variants)
    {
        names.emplace_back(entry.Name);
    }
    return winrt::single_threaded_vector<winrt::hstring>(std::move(names)).GetView();
}
