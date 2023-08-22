// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsSourceView.h"
#include "InspectingDataSource.h"

class ItemsSourceViewFactory :
    public winrt::implements<ItemsSourceViewFactory, winrt::IActivationFactory, winrt::IItemsSourceViewFactory>
{
public:

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<winrt::ItemsSourceView>() };
    }

    winrt::IInspectable ActivateInstance() const
    {
        throw winrt::hresult_not_implemented();
    }

    // ItemsSourceView's ctor creates an instance of the derived type InspectingDataSource.
    winrt::ItemsSourceView CreateInstance(winrt::IInspectable const& source, winrt::IInspectable const& baseInterface, winrt::IInspectable const& innerInterface)
    {
        assert(!baseInterface);
        assert(!innerInterface);
        return winrt::make<InspectingDataSource>(source);
    }
};

