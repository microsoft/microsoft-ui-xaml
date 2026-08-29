// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basic ctor
//
//------------------------------------------------------------------------
CSwapChainBackgroundPanel::CSwapChainBackgroundPanel(_In_ CCoreServices *pCore)
    : CSwapChainPanel(pCore)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basic dtor
//
//------------------------------------------------------------------------
CSwapChainBackgroundPanel::~CSwapChainBackgroundPanel()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Throw exception on setting unsupported property values on SCBP
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainBackgroundPanel::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::UIElement_Clip:
        case KnownPropertyIndex::UIElement_Opacity:
        case KnownPropertyIndex::UIElement_RenderTransform:
        case KnownPropertyIndex::UIElement_Projection:
        case KnownPropertyIndex::UIElement_RenderTransformOrigin:
        case KnownPropertyIndex::FrameworkElement_Width:
        case KnownPropertyIndex::FrameworkElement_Height:
        case KnownPropertyIndex::FrameworkElement_MinWidth:
        case KnownPropertyIndex::FrameworkElement_MaxWidth:
        case KnownPropertyIndex::FrameworkElement_MinHeight:
        case KnownPropertyIndex::FrameworkElement_MaxHeight:
        case KnownPropertyIndex::FrameworkElement_Margin:
        case KnownPropertyIndex::Panel_Background:
            //
            // Disallow setting layout affecting properties that do not apply to SCBP which is sized to render target size.
            //
            IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                GetContext(),
                AG_E_SWAPCHAINBACKGROUNDPANEL_ERROR_SETUNSUPPORTEDPROPERTY,
                args.m_pDP->GetName()));
            break;
        }
    }

    IFC_RETURN(CSwapChainPanel::SetValue(args));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Force a layout clip on the immediate children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainBackgroundPanel::PreRender()
{
    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        IFC_RETURN(pChild->UpdateLayoutClip(TRUE /*forceClipToRenderSize*/));
    }

    return S_OK;
}
