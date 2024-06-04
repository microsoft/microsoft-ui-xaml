// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Layout.h"
#include "VirtualizingLayout.g.h"

class VirtualizingLayout :
    public winrt::implementation::VirtualizingLayoutT<VirtualizingLayout, Layout>
{
public:
    VirtualizingLayout();

#pragma region IVirtualizingLayoutOverrides
    virtual void InitializeForContextCore(winrt::VirtualizingLayoutContext const& context);
    virtual void UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context);

    virtual winrt::Size MeasureOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& availableSize);
    virtual winrt::Size ArrangeOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& finalSize);

    virtual void OnItemsChangedCore(winrt::VirtualizingLayoutContext const& context, winrt::IInspectable const& source, winrt::NotifyCollectionChangedEventArgs const& args);
#pragma endregion
};
