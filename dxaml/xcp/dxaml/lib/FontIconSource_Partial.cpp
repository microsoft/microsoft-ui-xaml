// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IconElement.g.h"
#include "FontIcon.g.h"
#include "FontIconSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
FontIconSource::CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    wrl_wrappers::HString glyph;
    DOUBLE fontSize;
    ctl::ComPtr<xaml_media::IFontFamily> fontFamily;
    wut::FontWeight fontWeight;
    wut::FontStyle fontStyle;
    BOOLEAN isTextScaleFactorEnabled;
    BOOLEAN mirroredWhenRightToLeft;
    
    IFC_RETURN(get_Glyph(glyph.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_FontSize(&fontSize));
    IFC_RETURN(get_FontWeight(&fontWeight));
    IFC_RETURN(get_FontStyle(&fontStyle));
    IFC_RETURN(get_IsTextScaleFactorEnabled(&isTextScaleFactorEnabled));
    IFC_RETURN(get_MirroredWhenRightToLeft(&mirroredWhenRightToLeft));
    IFC_RETURN(get_FontFamily(&fontFamily));
    
    ctl::ComPtr<FontIcon> fontIcon;
    IFC_RETURN(ctl::make(&fontIcon));
    
    IFC_RETURN(fontIcon->put_Glyph(glyph.Get()));
    IFC_RETURN(fontIcon->put_FontSize(fontSize));
    IFC_RETURN(fontIcon->put_FontWeight(fontWeight));
    IFC_RETURN(fontIcon->put_FontStyle(fontStyle));
    IFC_RETURN(fontIcon->put_IsTextScaleFactorEnabled(isTextScaleFactorEnabled));
    IFC_RETURN(fontIcon->put_MirroredWhenRightToLeft(mirroredWhenRightToLeft));

    if (fontFamily)
    {
        IFC_RETURN(fontIcon->put_FontFamily(fontFamily.Get()));
    }

    *returnValue = fontIcon.Detach();
    return S_OK;
}

_Check_return_ HRESULT
FontIconSource::GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue)
{
    ctl::ComPtr<DependencyPropertyHandle> iconSourcePropertyHandle;
    IFC_RETURN(ctl::do_query_interface(iconSourcePropertyHandle, iconSourceProperty));
    
    switch (iconSourcePropertyHandle->GetDP()->GetIndex())
    {
    case KnownPropertyIndex::FontIconSource_FontFamily:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_FontFamily, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_FontSize:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_FontSize, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_FontStyle:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_FontStyle, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_FontWeight:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_FontWeight, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_Glyph:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_Glyph, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_IsTextScaleFactorEnabled:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_IsTextScaleFactorEnabled, returnValue));
        break;
    case KnownPropertyIndex::FontIconSource_MirroredWhenRightToLeft:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FontIcon_MirroredWhenRightToLeft, returnValue));
        break;
    default:
        IFC_RETURN(FontIconSourceGenerated::GetIconElementPropertyCoreImpl(iconSourceProperty, returnValue));
    }
    
    return S_OK;
}