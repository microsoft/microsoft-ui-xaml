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

    struct ContextMenuCandidate
    {
        winrt::FlyoutBase Flyout{ nullptr };
        winrt::FrameworkElement Anchor{ nullptr };

        explicit operator bool() const noexcept { return static_cast<bool>(Flyout); }
    };

    // True when a header or row cell anchor is still the visible cell for column, directly under
    // its cells host.
    bool IsLiveCellAnchor(
        winrt::FrameworkElement const& anchor,
        winrt::Panel const& scopeRoot,
        winrt::TableViewColumn const& column);

    // True when a request's OriginalSource is scope or inside it without crossing a nested
    // TableView or TableViewRow: a request from a nested table belongs to that table, even when it
    // left it Unhandled.
    bool IsOwnRequestSource(winrt::DependencyObject const& source, winrt::DependencyObject const& scope);
}
