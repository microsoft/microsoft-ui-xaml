// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RootScrollViewer.g.h"
#include "FlyoutBase.g.h"
#include "ContentDialog.g.h"
#include "IApplicationBarService.h"
#include "ContentDialogMetadata.h"
#include "XamlRoot_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the RootScrollViewer class.
RootScrollViewer::RootScrollViewer()
    : m_isInputPaneShow(FALSE)
    , m_isInputPaneTransit(FALSE)
    , m_isInputPaneTransitionCompleted(FALSE)
    , m_isAllowImplicitStyle(FALSE)
    , m_preInputPaneOffsetX(0.0f)
    , m_preInputPaneOffsetY(0.0f)
    , m_postInputPaneOffsetX(0.0f)
    , m_postInputPaneOffsetY(0.0f)
{
}

// Destroys an instance of the RootScrollViewer class.
RootScrollViewer::~RootScrollViewer()
{
}

//-------------------------------------------------------------------------
//
//  Function:   RootScrollViewer::NotifyInputPaneShowing
//
//  Synopsis:
//    Called to let the peer know that InputPane is showing. 
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
RootScrollViewer::NotifyInputPaneStateChange(
    _In_ InputPaneState inputPaneState,
    _In_ XRECTF inputPaneBounds)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IApplicationBarService> applicationBarService;
    ctl::ComPtr<FlyoutMetadata> spFlyoutMetadata;
    ctl::ComPtr<IFlyoutBase> spOpenFlyout;
    ctl::ComPtr<IFrameworkElement> spOpenFlyoutPlacementTarget;
    ctl::ComPtr<ContentDialogMetadata> spContentDialogMetadata;
    ctl::ComPtr<XamlRoot> xamlRoot = XamlRoot::GetImplementationForElementStatic(this);
    BOOLEAN isInputPaneShow = inputPaneState != InputPaneState::InputPaneHidden;
    
    if (inputPaneState == InputPaneState::InputPaneShowing || inputPaneState == InputPaneState::InputPaneHidden)
    {
        if (m_trElementScrollContentPresenter.Get())
        {
            m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->NotifyInputPaneStateChange(isInputPaneShow);
        }

        if (isInputPaneShow && !m_isInputPaneShow)
        {
            IFC(get_HorizontalOffset(&m_preInputPaneOffsetX));
            IFC(get_VerticalOffset(&m_preInputPaneOffsetY));
        }

        if (isInputPaneShow)
        {
            // Set VerticalScrollMode/HorizontalScrollMode to Enabled so the content can be scrolled while the input pane is shown.
            IFC(put_VerticalScrollMode(xaml_controls::ScrollMode_Enabled));
            IFC(put_HorizontalScrollMode(xaml_controls::ScrollMode_Enabled));
    #ifdef DBG
            // IsVerticalRailEnabled/IsHorizontalRailEnabled are expected to be True
            BOOLEAN isRailEnabled = FALSE;
            IFC(get_IsVerticalRailEnabled(&isRailEnabled));
            ASSERT(isRailEnabled);
            IFC(get_IsHorizontalRailEnabled(&isRailEnabled));
            ASSERT(isRailEnabled);
    #endif // DBG
        }
        else 
        {
            // Disabled the ScrollMode when IHM is closed
            IFC(put_VerticalScrollMode((xaml_controls::ScrollMode_Disabled)));
            IFC(put_HorizontalScrollMode((xaml_controls::ScrollMode_Disabled)));
        }
        
        if (xamlRoot)
        {
            IFC(xamlRoot->TryGetApplicationBarService(applicationBarService));

            if (applicationBarService)
            {
                IFC(applicationBarService->OnBoundsChanged(TRUE));
            }
        }
    }

    // Ensure Flyout size and position properly with showing/hiding IHM.
    IFC(DXamlCore::GetCurrent()->GetFlyoutMetadata(&spFlyoutMetadata));
    ASSERT(spFlyoutMetadata.Get() != nullptr);
    IFC(spFlyoutMetadata->GetOpenFlyout(&spOpenFlyout, &spOpenFlyoutPlacementTarget));
    if (spOpenFlyout)
    {
        IFC(spOpenFlyout.Cast<FlyoutBase>()->NotifyInputPaneStateChange(inputPaneState, inputPaneBounds));
    }

    // Update ContentDialog size/position with showing/hiding IHM.
    // We wont fail if xamlRoot is null because if RSV is not
    // in visual tree, it simply means that it's not attached yet.
    if (xamlRoot)
    {
        IFC(xamlRoot->GetContentDialogMetadata(&spContentDialogMetadata));
        IFC(spContentDialogMetadata->ForEachOpenDialog([inputPaneState, inputPaneBounds](xaml_controls::IContentDialog* openDialog, bool& /*stopIterating*/)
        {
            return static_cast<ContentDialog*>(openDialog)->NotifyInputPaneStateChange(inputPaneState, inputPaneBounds);
        }));
    }

    m_isInputPaneShow = isInputPaneShow;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   RootScrollViewer::ApplyInputPaneTransition
//
//  Synopsis:
//    Called to let the peer know that InputPane ready to transit. 
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
RootScrollViewer::ApplyInputPaneTransition(_In_ BOOLEAN isInputPaneTransitionEnabled)
{
    HRESULT hr = S_OK;

    m_isInputPaneTransit = isInputPaneTransitionEnabled;

    if (m_trElementScrollContentPresenter.Get())
    {
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->ApplyInputPaneTransition(isInputPaneTransitionEnabled));
    }

    // InputPane is hiding
    if (!m_isInputPaneShow)
    {
        BOOLEAN isOffsetUpdated = FALSE;
        DOUBLE xOffset = 0.0;
        DOUBLE yOffset = 0.0;

        // Restore the scroll horizontal and vertical offset
        IFC(get_HorizontalOffset(&xOffset));
        IFC(get_VerticalOffset(&yOffset));

        if (m_preInputPaneOffsetX != xOffset && m_postInputPaneOffsetX == xOffset)
        {
            IFC(HandleHorizontalScroll(xaml_primitives::ScrollEventType_ThumbPosition, m_preInputPaneOffsetX));
            isOffsetUpdated = TRUE;
        }
        if (m_preInputPaneOffsetY != yOffset && m_postInputPaneOffsetY == yOffset)
        {
            IFC(HandleVerticalScroll(xaml_primitives::ScrollEventType_ThumbPosition, m_preInputPaneOffsetY));
            isOffsetUpdated = TRUE;
        }
        if (isOffsetUpdated)
        {
            IFC(UpdateLayout());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Update the InputPane horizontal offset.
_Check_return_ HRESULT 
RootScrollViewer::UpdateInputPaneOffsetX()
{
    HRESULT hr = S_OK;
    
    if (m_isInputPaneTransit)
    {
        IFC(get_HorizontalOffset(&m_postInputPaneOffsetX));
        m_isInputPaneTransitionCompleted = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// Update the InputPane vertical offset.
_Check_return_ HRESULT 
RootScrollViewer::UpdateInputPaneOffsetY()
{
    HRESULT hr = S_OK;
    
    if (m_isInputPaneTransit)
    {
        IFC(get_VerticalOffset(&m_postInputPaneOffsetY));
        m_isInputPaneTransitionCompleted = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// Update the InputPane transition status
void
RootScrollViewer::UpdateInputPaneTransition()
{
    if (m_isInputPaneTransitionCompleted)
    {
        m_isInputPaneTransit = FALSE;
        m_isInputPaneTransitionCompleted = FALSE;
    }
}

// RootScrollViewer prevent to show the root ScrollViewer automation peer.
IFACEMETHODIMP 
RootScrollViewer::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    RRETURN(S_OK);
}
