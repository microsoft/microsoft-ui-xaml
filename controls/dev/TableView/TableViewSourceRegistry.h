// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewSourceRegistry.g.h"

// Creates a TableView source by variant name. Each variant is a class implementing both the public
// ITableViewShapingSource verbs and the internal ::ITableViewSource contract TableView binds through;
// adding one is a registry entry in the .cpp, with no IDL change.
class TableViewSourceRegistry
{
public:
    static winrt::ITableViewShapingSource Create(winrt::hstring const& variant, winrt::IInspectable const& items);
    static winrt::ITableViewShapingSource Create(winrt::hstring const& variant, winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector);
    static winrt::IVectorView<winrt::hstring> Variants();
};
