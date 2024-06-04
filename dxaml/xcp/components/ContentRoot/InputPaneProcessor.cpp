// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "InputPaneProcessor.h"
#include "ContentRoot.h"
#include "RootVisual.h"
#include "BringIntoViewHandler.h"

#include "InputPaneHandler.h"
#include "FrameworkInputViewHandler.h"

#include "ScrollContentControl.h"
#include "XcpInputPaneHandler.h"
#include "ScrollContentControl.h"
#include "PALInputPaneInteraction.h"

#include "corep.h"
#include "InputServices.h"
#include <FxCallbacks.h>

using namespace ContentRootInput;

InputPaneProcessor::InputPaneProcessor(_In_ CInputManager& inputManager)
    : m_inputManager(inputManager)
{
}

bool InputPaneProcessor::IsSipOpen() const
{
    return IsInputPaneShowing() || IsInputPaneShowingBringIntoViewNotHandled();
}

bool InputPaneProcessor::IsInputPaneShowing() const
{
    return m_pInputPaneHandler && static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get())->IsInputPaneShowed();
}

bool InputPaneProcessor::IsInputPaneShowingBringIntoViewNotHandled() const
{
    return m_pInputPaneHandler && static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get())->IsInputPaneShowedBringIntoViewNotHandled();
}

void InputPaneProcessor::CreateInputPaneHandler()
{
    if (m_pInputPaneHandler)
    {
        return;
    }

    const auto contentRoot = m_inputManager.GetContentRoot();
    if (m_pRegisteredRootScrollViewer = static_cast<CScrollContentControl*>(contentRoot->GetVisualTreeNoRef()->GetRootScrollViewer()))
    {
        m_pInputPaneHandler = std::make_unique<CInputPaneHandler>(m_pRegisteredRootScrollViewer.get(), contentRoot->ShouldUseVisualRelativePixels());
    }
}

_Check_return_ HRESULT InputPaneProcessor::RegisterInputPaneHandler(_In_opt_ XHANDLE hCoreWindow)
{
    // Register the input pane handler in case of having the CoreWindow handle.
    // CoreWindow handle can be null in case of SearchPane scenario.
    if (auto coreWindow = m_inputManager.m_coreServices.GetInputServices()->GetCoreWindow())
    {
        // m_pInputPaneHandler can be null when the visual tree doesn't include
        // the root SV in case of Canvas is the visual root.
        if (!m_pInputPaneInteraction && m_pInputPaneHandler)
        {
            IFC_RETURN(gps->GetInputPaneInteraction(m_pInputPaneHandler.get(), m_pInputPaneInteraction.ReleaseAndGetAddressOf()));

            // Register InputPane handler
            IFC_RETURN(m_pInputPaneInteraction->RegisterInputPaneHandler(coreWindow, m_inputManager.GetContentRoot()));
        }
    }

    return S_OK;
}

void InputPaneProcessor::UnregisterInputPaneHandler()
{
    // Unregister InputPane handler
    if (m_pInputPaneInteraction)
    {
        // InputPaneInteraction UnregisterInputPaneHandler can be failed by
        // calling it after destroying the core window.
        IGNOREHR(m_pInputPaneInteraction->UnregisterInputPaneHandler());
        m_pInputPaneInteraction = nullptr;
    }
}

void InputPaneProcessor::DestroyInputPaneHandler()
{
    UnregisterInputPaneHandler();

    m_pInputPaneHandler.reset();
    m_pRegisteredRootScrollViewer = nullptr;
}

IXcpInputPaneHandler* InputPaneProcessor::GetInputPaneHandler() const
{
    return m_pInputPaneHandler.get();
}

IPALInputPaneInteraction* InputPaneProcessor::GetInputPaneInteraction() const
{
    return m_pInputPaneInteraction.get();
}

DirectUI::InputPaneState InputPaneProcessor::GetInputPaneState() const
{
    return m_pInputPaneHandler ? (static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get()))->GetInputPaneState() : DirectUI::InputPaneState::InputPaneHidden;
}

_Check_return_ HRESULT InputPaneProcessor::GetInputPaneBounds(_Out_ XRECTF* pInputPaneBounds) const
{
    if (m_pInputPaneHandler)
    {
        IFC_RETURN(static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get())->GetInputPaneBounds(pInputPaneBounds));
    }
    else
    {
        pInputPaneBounds->X = 0.0f;
        pInputPaneBounds->Y = 0.0f;
        pInputPaneBounds->Width = 0.0f;
        pInputPaneBounds->Height = 0.0f;
    }

    return S_OK;
}

void InputPaneProcessor::AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height)
{
    VisualTree* visualTreeNoRef = nullptr;

    if (!m_pInputPaneHandler)
    {
        visualTreeNoRef = m_inputManager.GetContentRoot()->GetVisualTreeNoRef();

        if (visualTreeNoRef && visualTreeNoRef->GetRootScrollViewer())
        {
            // Create a CInputPaneHandler when there is a root ScrollViewer.
            // This situation can occur when this method is called by CTextBoxBase::BringLastVisibleRectIntoView.
            CreateInputPaneHandler();
        }
    }

    if (m_pInputPaneHandler)
    {
        (static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get()))->AdjustBringIntoViewRecHeight(topGlobal, bottomGlobal, height);
    }
    else
    {
        // This branch is used when the root element is a Canvas instead of a ScrollViewer. In those cases m_pInputPaneHandler and its m_pRootScrollViewer
        // do not exist. The static class CBringIntoViewHandler is used directly without the CInputPaneHandler intermediary.

        ASSERT(visualTreeNoRef && !visualTreeNoRef->GetRootScrollViewer());

        CUIElement* canvasNoRef = visualTreeNoRef->GetPublicRootVisual();

        if (canvasNoRef)
        {
            ASSERT(canvasNoRef->OfTypeByIndex<KnownTypeIndex::Canvas>());

            wf::Rect contentBounds{};

            IFCFAILFAST(FxCallbacks::DXamlCore_GetContentBoundsForElement(canvasNoRef, &contentBounds));

            CBringIntoViewHandler::AdjustBringIntoViewRecHeight(
                canvasNoRef /* rootElement */,
                contentBounds.Height,
                topGlobal,
                bottomGlobal,
                height);
        }
    }
}

_Check_return_ HRESULT InputPaneProcessor::RegisterFrameworkInputView()
{
    if (!m_spFrameworkInputViewHandler)
    {
        const auto contentRoot = m_inputManager.GetContentRoot();
        m_spFrameworkInputViewHandler.attach(new CFrameworkInputViewHandler(contentRoot->GetFocusManagerNoRef()));
        IFC_RETURN(m_spFrameworkInputViewHandler->Initialize());
    }

    return S_OK;
}

_Check_return_ HRESULT InputPaneProcessor::NotifyFocusChanged(_In_opt_ CDependencyObject* pFocusedElement, _In_ bool bringIntoView, _In_ bool animateIfBringIntoView)
{
    if (pFocusedElement)
    {
        CInputPaneHandler* pInputPaneHandler = nullptr;
        bool forceBringIntoView = false;

        if (!m_pInputPaneHandler)
        {
            CreateInputPaneHandler();
        }

        // MSFT: 19438099
        XHANDLE coreWindow = m_inputManager.m_coreServices.GetInputServices()->GetCoreWindow();

        // Register InputPane handler when the focus is on the text control or WebView that
        // can support the bring into view for the focused element.
        if (coreWindow && !m_pInputPaneInteraction && CanUseInputPane(static_cast<CDependencyObject*>(pFocusedElement)))
        {
            // Note: We don't check for the open pane because if it had been opened while the application
            // was running, we would have previously registered the handler when we were told the pane was
            // opening.  It might have been open since before the application started, but checking is a
            // very expensive operation (in terms of cocreates/memory) and the scenario is rare.
            IFC_RETURN(RegisterInputPaneHandler(coreWindow));
        }

        pInputPaneHandler = static_cast<CInputPaneHandler*>(m_pInputPaneHandler.get());

        // Caller specifies bringIntoView=TRUE only for controls that gain
        // focus using keyboard. In addition, bring a focused text editable control
        // into view when the input pane is showing, regardless of how the text editable
        // control gained focus, so user can see the control while entering text
        if (pInputPaneHandler &&
            pInputPaneHandler->IsInputPaneShowed() &&
            m_inputManager.m_coreServices.GetInputServices()->IsTextEditableControl(static_cast<CDependencyObject*>(pFocusedElement)))
        {
            bringIntoView = true;
            // Force into view even if app has set ScrollViewer.BringIntoViewOnFocusChange to FALSE,
            // to reduce risk near ship date, in case some apps want Text controls to scroll into view
            // when the Input Pane is showing, even when ScrollViewer.BringIntoViewOnFocusChange
            // is set to FALSE. ScrollViewer.BringIntoViewOnFocusChange = FALSE is currently used
            // to allow an app to prevent BringIntoView when an element gains focus using
            // the keyboard.
            forceBringIntoView = true;
        }

        if (bringIntoView)
        {
            if (pInputPaneHandler)
            {
                IFC_RETURN(pInputPaneHandler->EnsureFocusedElementBringIntoView(false /*isIHMShowing*/, forceBringIntoView, animateIfBringIntoView));
            }
            else
            {
                // This branch is used when the root element is a Canvas instead of a ScrollViewer. In those cases m_pInputPaneHandler and its m_pRootScrollViewer
                // do not exist. The static class CBringIntoViewHandler is used directly without the CInputPaneHandler intermediary.
                VisualTree* visualTreeNoRef = m_inputManager.GetContentRoot()->GetVisualTreeNoRef();

                ASSERT(visualTreeNoRef && !visualTreeNoRef->GetRootScrollViewer());

                CUIElement* canvasNoRef = visualTreeNoRef->GetPublicRootVisual();

                if (canvasNoRef)
                {
                    ASSERT(canvasNoRef->OfTypeByIndex<KnownTypeIndex::Canvas>());

                    wf::Rect contentBounds{};

                    IFC_RETURN(FxCallbacks::DXamlCore_GetContentBoundsForElement(canvasNoRef, &contentBounds));

                    IFC_RETURN(CBringIntoViewHandler::EnsureFocusedElementBringIntoView(
                        pFocusedElement,
                        canvasNoRef /*rootElement*/,
                        contentBounds.Height /*rootHeightInDips*/,
                        0.0f,
                        0.0f /*ihmTopInDips*/,
                        GetInputPaneState(),
                        false,
                        forceBringIntoView,
                        animateIfBringIntoView));
                }
            }
        }
    }

    return S_OK;
}

bool InputPaneProcessor::CanUseInputPane(_In_ CDependencyObject *pObject)
{
    return pObject && (pObject->OfTypeByIndex<KnownTypeIndex::TextBox>() ||
                       pObject->OfTypeByIndex<KnownTypeIndex::PasswordBox>() ||
                       pObject->OfTypeByIndex<KnownTypeIndex::RichEditBox>());
}