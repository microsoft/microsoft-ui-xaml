// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a sharable icon source that can be turned into an IconElement.

#pragma once

#include "PathIconSource.g.h"

namespace DirectUI
{
    class __declspec(uuid(__PathIconSource_GUID)) PathIconSource : public PathIconSourceGenerated
    {
    public:
        _Check_return_ HRESULT CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue) override;
        _Check_return_ HRESULT GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue) override;
    };
}