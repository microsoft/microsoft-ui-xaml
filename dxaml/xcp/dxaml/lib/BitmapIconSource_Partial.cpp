// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IconElement.g.h"
#include "BitmapIcon.g.h"
#include "BitmapIconSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
BitmapIconSource::CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    ctl::ComPtr<wf::IUriRuntimeClass> uriSource;
    BOOLEAN showAsMonochrome;
    
    IFC_RETURN(get_UriSource(&uriSource));
    IFC_RETURN(get_ShowAsMonochrome(&showAsMonochrome));
    
    ctl::ComPtr<BitmapIcon> bitmapIcon;
    IFC_RETURN(ctl::make(&bitmapIcon));
    
    IFC_RETURN(bitmapIcon->put_UriSource(uriSource.Get()));
    IFC_RETURN(bitmapIcon->put_ShowAsMonochrome(showAsMonochrome));

    *returnValue = bitmapIcon.Detach();
    return S_OK;
}

_Check_return_ HRESULT
BitmapIconSource::GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue)
{
    ctl::ComPtr<DependencyPropertyHandle> iconSourcePropertyHandle;
    IFC_RETURN(ctl::do_query_interface(iconSourcePropertyHandle, iconSourceProperty));
    
    switch (iconSourcePropertyHandle->GetDP()->GetIndex())
    {
    case KnownPropertyIndex::BitmapIconSource_ShowAsMonochrome:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::BitmapIcon_ShowAsMonochrome, returnValue));
        break;
    case KnownPropertyIndex::BitmapIconSource_UriSource:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::BitmapIcon_UriSource, returnValue));
        break;
    default:
        IFC_RETURN(BitmapIconSourceGenerated::GetIconElementPropertyCoreImpl(iconSourceProperty, returnValue));
    }
    
    return S_OK;
}