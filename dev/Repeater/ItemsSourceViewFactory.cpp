// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsSourceViewFactory.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithFactory(ItemsSourceView, ItemsSourceViewFactory)
}

void* winrt_make_Microsoft_UI_Xaml_Controls_ItemsSourceView()
{
    return winrt::detach_abi(winrt::make<winrt::Microsoft::UI::Xaml::Controls::factory_implementation::ItemsSourceView>());
}
namespace winrt::Microsoft::UI::Xaml::Controls
{
    ItemsSourceView::ItemsSourceView(Windows::Foundation::IInspectable const& source) :
        ItemsSourceView(winrt::make<InspectingDataSource>(source))
    {
    }
}
