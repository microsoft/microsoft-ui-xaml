// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents RootScrollViewer to scroll the content during IHM showing.

#pragma once

#include "RootScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(RootScrollViewer)
    {

    public:
        // Called when InputPane is showing.
        _Check_return_ HRESULT NotifyInputPaneStateChange(_In_ DirectUI::InputPaneState inputPaneState, _In_ XRECTF inputPaneBounds);
        _Check_return_ HRESULT ApplyInputPaneTransition(_In_ BOOLEAN isEnableThemeTransition);
        void SetRootScrollViewerAllowImplicitStyle()
        {
            m_isAllowImplicitStyle = TRUE;
        }

        void SetRootScrollContentPresenter(_In_ ScrollContentPresenter* const pScrollContentPresenter)
        { 
            m_trElementScrollContentPresenter.Clear();
            SetPtrValue(m_trElementScrollContentPresenter, pScrollContentPresenter);
        }

    protected:
        // Initializes a new instance of the RootScrollViewer class.
        RootScrollViewer();

        // Destroys an instance of the RootScrollViewer class.
        ~RootScrollViewer() override;

        _Check_return_ HRESULT UpdateInputPaneOffsetX() override;
        _Check_return_ HRESULT UpdateInputPaneOffsetY() override;
        void UpdateInputPaneTransition() override;

        BOOLEAN IsRootScrollViewer() override
        { return TRUE; }

        BOOLEAN IsRootScrollViewerAllowImplicitStyle() override
        { return m_isAllowImplicitStyle; }

        BOOLEAN IsInputPaneShow() override
        { return m_isInputPaneShow; }

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

    private:

        // Indicates the root ScrollViewer for InputPane
        BOOLEAN m_isInputPaneShow;
        BOOLEAN m_isInputPaneTransit;
        BOOLEAN m_isInputPaneTransitionCompleted;
        BOOLEAN m_isAllowImplicitStyle;

        // InputPane offset variables to restore the original scroll position when InputPane is closed
        DOUBLE m_preInputPaneOffsetX;
        DOUBLE m_preInputPaneOffsetY;
        DOUBLE m_postInputPaneOffsetX;
        DOUBLE m_postInputPaneOffsetY;
    };
}
