// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Used as a root for media swapchain and child controls (if
//  applicable) while MediaElement is in Full Window Mode.

#include "precomp.h"
#include "XboxUtility.h"

CFullWindowMediaRoot::~CFullWindowMediaRoot()
{
    SAFE_DELETE_ARRAY(m_ppRenderChildren);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the Panel Background brush to opaque black SolidColorBrush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFullWindowMediaRoot::InitInstance()
{
    HRESULT hr = S_OK;
    CSolidColorBrush* pBackgroundBrush = NULL;
    CValue val;
    CREATEPARAMETERS cp(GetContext());

    // Set Visibility to Collapsed. The FullWindowMediaRoot takes up the whole app
    // window and has a high z-order. When not collapsed, it blocks the entire visual 
    // root. Hence it is only made visible if there is full-window MediaElement. 
    val.SetEnum(static_cast<XUINT32>(DirectUI::Visibility::Collapsed));
    IFC(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, val));

    // Set Panel Background to opaque black
    IFC(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(&pBackgroundBrush), &cp));
    val.SetColor(0xFF000000 /* opaque black */);
    IFC(pBackgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, val));

    IFC(SetValueByKnownIndex(KnownPropertyIndex::Panel_Background, pBackgroundBrush));

Cleanup:
    ReleaseInterfaceNoNULL(pBackgroundBrush);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to include the MediaEngineSwapChainElement in the children collection
//      for rendering and hit-testing.
//
//------------------------------------------------------------------------
void
CFullWindowMediaRoot::GetChildrenInRenderOrder(
    _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
    CUIElement **ppPublicChildren = nullptr;
    XUINT32 publicChildrenCount = 0;

    CPanel::GetChildrenInRenderOrder(&ppPublicChildren, &publicChildrenCount);

    {
        *pppUIElements = ppPublicChildren;
        *puiChildCount = publicChildrenCount;
    }
}

_Check_return_ HRESULT
CFullWindowMediaRoot::MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    desiredSize = availableSize;

#if false
    XRECTF contentRootBounds = {};
    IFC_RETURN(FxCallbacks::Window_GetContentRootLayoutBounds(this, &contentRootBounds));

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 i = 0; i < count; i++)
    {
        XSIZEF childControlSize = { contentRootBounds.Width, contentRootBounds.Height };

        IFC_RETURN(children[i]->EnsureLayoutStorage());
        IFC_RETURN(children[i]->Measure(childControlSize));
    }
#endif

    return S_OK;
}

_Check_return_ HRESULT
CFullWindowMediaRoot::ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize)
{
    newFinalSize = finalSize;
    
#if false
    XRECTF contentRootBounds = {};
    IFC_RETURN(FxCallbacks::Window_GetContentRootLayoutBounds(this, &contentRootBounds));

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    XRECTF childControlRect = {0.0f, 0.0f, 0.0f, 0.0f};
    for (XUINT32 i = 0; i < count; i++)
    {
        // Set the child control width/height.
        // The SystemTray and AppBar will be suppressed on the full windowed media mode, 
        // so the top position is set as zero.

        // On XBox arrange the children with the full screen size, including the TV safe regions.
        if (XboxUtility::IsOnXbox())
        {
            if (!m_pFullWindowMediaElementNoRef) // MediaPlayerElement
            {
                childControlRect.Width = contentRootBounds.Width + 2 * contentRootBounds.X;
                childControlRect.Height = contentRootBounds.Height + 2 * contentRootBounds.Y;
            }
            else // MediaElement
            {
                childControlRect.X = contentRootBounds.X;
                childControlRect.Y = contentRootBounds.Y;
                childControlRect.Width = contentRootBounds.Width;
                childControlRect.Height = contentRootBounds.Height;
            }
        }
        else
        {
            childControlRect.Width = contentRootBounds.Width;
            childControlRect.Height = contentRootBounds.Height;
        }

        IFC_RETURN(children[i]->EnsureLayoutStorage());
        IFC_RETURN(children[i]->Arrange(childControlRect));
    }
#endif

    return S_OK;
}

