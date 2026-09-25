// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

namespace TableViewDetails
{
    enum class ContextMenuTargetKind
    {
        Body,
        Header,
    };

    struct ContextMenuTarget
    {
        ContextMenuTargetKind Kind{ ContextMenuTargetKind::Body };
        winrt::TableViewRow Row{ nullptr };
        winrt::IInspectable Item{ nullptr };
        winrt::TableViewColumn Column{ nullptr };
        winrt::FrameworkElement Anchor{ nullptr };
        winrt::Panel ScopeRoot{ nullptr };
    };

    enum class ContextMenuResult
    {
        Unhandled,
        Suppressed,
        Shown,
    };
}
