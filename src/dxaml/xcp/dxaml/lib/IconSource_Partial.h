// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a sharable icon source that can be turned into an IconElement.

#pragma once

#include "IconSource.g.h"

namespace DirectUI
{
    class __declspec(uuid(__IconSource_GUID)) IconSource : public IconSourceGenerated
    {
    public:
        IFACEMETHOD(CreateIconElement)(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue) override;
        
        virtual _Check_return_ HRESULT CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue);
        virtual _Check_return_ HRESULT GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue);
        
    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        
    private:
        std::vector<ctl::WeakRefPtr> m_createdIconElements{};
    };
}