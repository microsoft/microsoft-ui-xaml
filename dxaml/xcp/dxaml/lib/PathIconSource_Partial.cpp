// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IconElement.g.h"
#include "PathIcon.g.h"
#include "PathIconSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
PathIconSource::CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    ctl::ComPtr<IGeometry> data;
    
    IFC_RETURN(get_Data(&data));
    
    ctl::ComPtr<PathIcon> pathIcon;
    IFC_RETURN(ctl::make(&pathIcon));
    
    IFC_RETURN(pathIcon->put_Data(data.Get()));

    *returnValue = pathIcon.Detach();
    return S_OK;
}

_Check_return_ HRESULT
PathIconSource::GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue)
{
    ctl::ComPtr<DependencyPropertyHandle> iconSourcePropertyHandle;
    IFC_RETURN(ctl::do_query_interface(iconSourcePropertyHandle, iconSourceProperty));
    
    switch (iconSourcePropertyHandle->GetDP()->GetIndex())
    {
    case KnownPropertyIndex::PathIconSource_Data:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PathIcon_Data, returnValue));
        break;
    default:
        IFC_RETURN(PathIconSourceGenerated::GetIconElementPropertyCoreImpl(iconSourceProperty, returnValue));
    }
    
    return S_OK;
}