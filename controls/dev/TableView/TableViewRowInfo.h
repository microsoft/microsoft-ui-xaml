// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <winrt/Windows.Foundation.h>

enum class TableViewRowKind
{
    Data = 0,
    GroupHeader = 1,
};

struct TableViewRowInfo
{
    TableViewRowKind Kind{ TableViewRowKind::Data };
    int32_t Level{ 0 };
    bool IsExpandable{ false };
    bool IsExpanded{ false };
};

using TableViewRowItemKeySelector = std::function<winrt::hstring(winrt::IInspectable const&)>;

struct ITableViewRowMetadataProvider
{
    virtual ~ITableViewRowMetadataProvider() = default;

    virtual TableViewRowInfo GetRowInfo(int32_t index) = 0;
    virtual winrt::hstring GetIdentity(int32_t index) = 0;
    virtual void Expand(winrt::hstring const& key) = 0;
    virtual void Collapse(winrt::hstring const& key) = 0;
    virtual bool Toggle(winrt::hstring const& key) = 0;

    virtual void ExpandGroupObject(winrt::IInspectable const& key) = 0;
    virtual void CollapseGroupObject(winrt::IInspectable const& key) = 0;
    virtual bool ToggleGroupObject(winrt::IInspectable const& key) = 0;
    virtual void ExpandAllGroups() = 0;
    virtual void CollapseAllGroups() = 0;
};

using TableViewRowMetadataProvider = std::shared_ptr<ITableViewRowMetadataProvider>;
