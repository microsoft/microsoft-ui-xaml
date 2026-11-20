// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IconElement.g.h"
#include "IconSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP IconSource::CreateIconElement(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    ctl::ComPtr<IIconElement> element;
    IFC_RETURN(IconSourceGenerated::CreateIconElement(&element));
    
    if (element)
    {
        ctl::ComPtr<IBrush> foreground;
        IFC_RETURN(get_Foreground(&foreground));
        
        // This check ensures that we are not forcing developers to provide a foreground, if no foreground is set, it picks the default foreground based on theme.
        if (foreground)
        {
            IFC_RETURN(element->put_Foreground(foreground.Get()));
        }
    }
    
    ctl::WeakRefPtr weakElement;
    IFC_RETURN(element.AsWeak(&weakElement));
    m_createdIconElements.push_back(weakElement);
    
    *returnValue = element.Detach();
    return S_OK;
}

_Check_return_ HRESULT IconSource::CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    // This is an abstract base type, so this should never be called.
    return E_NOTIMPL;
}

_Check_return_ HRESULT IconSource::GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue)
{
    ctl::ComPtr<DependencyPropertyHandle> iconSourcePropertyHandle;
    IFC_RETURN(ctl::do_query_interface(iconSourcePropertyHandle, iconSourceProperty));
    
    switch (iconSourcePropertyHandle->GetDP()->GetIndex())
    {
    case KnownPropertyIndex::IconSource_Foreground:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::IconElement_Foreground, returnValue));
        break;
    default:
        *returnValue = nullptr;
    }
    
    return S_OK;
}

_Check_return_ HRESULT IconSource::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    ctl::ComPtr<IDependencyProperty> iconSourceProperty;
    ctl::ComPtr<IDependencyProperty> iconElementProperty;
    
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(args.m_pDP->GetIndex(), &iconSourceProperty));
    IFC_RETURN(GetIconElementPropertyCoreProtected(iconSourceProperty.Get(), &iconElementProperty));
    
    if (iconElementProperty)
    {
        // Core-initiated changes (e.g. a ThemeResource re-resolved on theme change) carry only the CValue; forwarding
        // a null outer value would null out properties such as FontIcon.FontFamily.
        ctl::ComPtr<IInspectable> unboxedNewValue;
        IInspectable* newValueNoRef = args.m_pNewValueOuterNoRef;
        if (newValueNoRef == nullptr && args.m_pNewValue != nullptr && !args.m_pNewValue->IsNullOrUnset())
        {
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &unboxedNewValue));
            newValueNoRef = unboxedNewValue.Get();
        }

        m_createdIconElements.erase(std::remove_if(m_createdIconElements.begin(), m_createdIconElements.end(),
            [iconElementProperty, newValue = newValueNoRef](ctl::WeakRefPtr weakElement)
        {
            auto iconElement = weakElement.AsOrNull<IconElement>();
            
            if (iconElement)
            {
                iconElement->SetValue(iconElementProperty.Get(), newValue);
            }
            
            return !iconElement;
        }), m_createdIconElements.end());
    }
    
    return S_OK;
}
