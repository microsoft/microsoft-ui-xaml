// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CScrollContentControl::CScrollContentControl(_In_ CCoreServices *pCore) : CContentControl(pCore)
{
    m_rootScrollViewerOriginalHeight = 0;
    m_rootScrollViewerSettings = 0;
    m_bRootScrollViewer = FALSE;
}

CScrollContentControl::~CScrollContentControl()
{
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//      RootSV need to notify OnApplyTemplate to hook up the scrolling
//      components.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CScrollContentControl::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    // Call the base class to enter all of this object's properties
    IFC_RETURN(CContentControl::EnterImpl(pNamescopeOwner, params));

    // Root ScrollViewer need to apply the template immediately to walk up
    // the tree for firing the Loaded event on the right time instead of delaying
    // it by Measure happening.
    if (m_bRootScrollViewer && HasRootScrollViewerApplyTemplate())
    {
        CContentPresenter *pContentPresenter = static_cast<CContentPresenter*>(GetFirstChildNoAddRef());
        ASSERT(pContentPresenter && pContentPresenter->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

        // Notify OnApplyTemplate to hook up the scrolling components.
        IFC_RETURN(FxCallbacks::FrameworkElement_OnApplyTemplate(pContentPresenter));

        SetRootScrollViewerSettingApplyTemplate(FALSE);
    }

    return S_OK;
}

_Check_return_ HRESULT CScrollContentControl::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    if (m_bRootScrollViewer)
    {
        // Ensure no default template is applied for the root ScrollViewer after Win8.
        ReleaseInterface(this->m_pTemplate);
    }
    else
    {
        IFC_RETURN(CContentControl::ApplyTemplate(fAddedVisuals));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CScrollContentControl::SetValue
//
//  Synopsis:
//      Detect changes in key values.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CScrollContentControl::SetValue(_In_ const SetValueParams& args)
{
    // RootScrollViewer doesn't allow for applying the template.
    if ((m_bRootScrollViewer) &&
        (args.m_pDP->GetIndex() == KnownPropertyIndex::Control_Template ||
         args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_ContentTemplate ||
         args.m_pDP->GetIndex() == KnownPropertyIndex::ContentControl_SelectedContentTemplate))
    {
        // Do not allow for applying the implicit style for RootScrollViewer.
        return S_OK;
    }

    IFC_RETURN(CContentControl::SetValue(args));

    return S_OK;
}
