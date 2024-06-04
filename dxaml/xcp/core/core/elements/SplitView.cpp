// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitView.h"

#include "EnumDefs.g.h"
#include "Activators.g.h"

#include "SplitViewTemplateSettings.h"
#include "SplitViewPaneClosingEventArgs.h"

#include "DoubleUtil.h"
#include "XamlTraceLogging.h"
#include "corep.h"

using namespace Focus;

class CSplitViewPaneClosingExecutor : public CXcpObjectBase<IPALExecuteOnUIThread>
{
public:
    CSplitViewPaneClosingExecutor(_In_ CSplitView *pSplitView, _In_ CSplitViewPaneClosingEventArgs *pPaneClosingEventArgs)
        : m_spSplitView(pSplitView)
        , m_spPaneClosingEventArgs(pPaneClosingEventArgs)
    {}

    _Check_return_ HRESULT Execute() override
    {
        if (m_spPaneClosingEventArgs->m_cancel)
        {
            m_spSplitView->OnCancelClosing();
            return S_OK;
        }

        CValue value;
        value.SetBool(FALSE);
        return m_spSplitView->SetValueByKnownIndex(KnownPropertyIndex::SplitView_IsPaneOpen, value);
    }

private:
    xref_ptr<CSplitView> m_spSplitView;
    xref_ptr<CSplitViewPaneClosingEventArgs> m_spPaneClosingEventArgs;

}; // class SplitViewClosingExecutor;

//
// CSplitView Implementation
//
CSplitView::~CSplitView()
{
    VERIFYHR(UnregisterEventHandlers());

    ReleaseInterface(m_pTemplateSettings);
}

_Check_return_ HRESULT
CSplitView::InitInstance()
{
    CREATEPARAMETERS cp(GetContext());
    IFC_RETURN(CSplitViewTemplateSettings::Create((CDependencyObject **)&m_pTemplateSettings, &cp));

    SetIsCustomType();

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(__super::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::SplitView_DisplayMode:
        {
            IFC_RETURN(RestoreSavedFocusElement());
            IFC_RETURN(FxCallbacks::Control_UpdateVisualState(this, true /*useTransitions*/));
        }
        break;

    case KnownPropertyIndex::SplitView_PanePlacement:
    case KnownPropertyIndex::SplitView_LightDismissOverlayMode:
        {
            IFC_RETURN(FxCallbacks::Control_UpdateVisualState(this, true /*useTransitions*/));
        }
        break;

    case KnownPropertyIndex::SplitView_OpenPaneLength:
    case KnownPropertyIndex::SplitView_CompactPaneLength:
        {
            IFC_RETURN(UpdateTemplateSettings());

            // Force the bindings in our VisualState animations to refresh by intentionally
            // passing in false for 'useTransitions.'
            IFC_RETURN(FxCallbacks::Control_UpdateVisualState(this, false /*useTransitions*/));
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::OnApplyTemplate()
{
    IFC_RETURN(UnregisterEventHandlers());

    // Clear any hold-overs from the previous template.
    m_spPaneClipRectangle.reset();
    m_spContentRoot.reset();
    m_spPaneRoot.reset();
    m_spLightDismissLayer.reset();

    IFC_RETURN(__super::OnApplyTemplate());

    xref_ptr<CDependencyObject> spTemplateChild = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"PaneClipRectangle"));
    IFC_RETURN(m_spPaneClipRectangle.reset(do_pointer_cast<CRectangleGeometry>(spTemplateChild.get())));

    spTemplateChild = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"ContentRoot"));
    IFC_RETURN(m_spContentRoot.reset(do_pointer_cast<CUIElement>(spTemplateChild.get())));

    spTemplateChild = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"PaneRoot"));
    IFC_RETURN(m_spPaneRoot.reset(do_pointer_cast<CUIElement>(spTemplateChild.get())));

    spTemplateChild = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"LightDismissLayer"));
    IFC_RETURN(m_spLightDismissLayer.reset(do_pointer_cast<CUIElement>(spTemplateChild.get())));

    IFC_RETURN(RegisterEventHandlers());
    IFC_RETURN(UpdateTemplateSettings());
    IFC_RETURN(FxCallbacks::Control_UpdateVisualState(this, true /*useTransitions*/));

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    CValue value;

    // Measure the pane content so that we can use the desired size in cases
    // where open pane length is set to Auto.
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::SplitView_Pane, &value));
    if (!value.IsNullOrUnset())
    {
        auto pPaneElement = do_pointer_cast<CUIElement>(value.AsObject());

        IFC_RETURN(pPaneElement->Measure(availableSize));

        m_paneMeasuredLength = pPaneElement->DesiredSize.width;
    }

    IFC_RETURN(__super::MeasureOverride(availableSize, desiredSize));

    IFC_RETURN(UpdateTemplateSettings());

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    CValue value;

    IFC_RETURN(__super::ArrangeOverride(finalSize, newFinalSize));

    if (m_spPaneClipRectangle)
    {
        XRECTF rect = { 0, 0, static_cast<float>(GetOpenPaneLength()), newFinalSize.height };
        value.WrapRect(&rect);
        IFC_RETURN(m_spPaneClipRectangle->SetValueByKnownIndex(KnownPropertyIndex::RectangleGeometry_Rect, value));
    }

    return S_OK;
}

bool CSplitView::IsLightDismissible()
{
    CValue value;
    return (SUCCEEDED(GetValueByIndex(KnownPropertyIndex::SplitView_DisplayMode, &value)) ?
            static_cast<DirectUI::SplitViewDisplayMode>(value.AsEnum()) != DirectUI::SplitViewDisplayMode::Inline &&
            static_cast<DirectUI::SplitViewDisplayMode>(value.AsEnum()) != DirectUI::SplitViewDisplayMode::CompactInline : false);
}

bool CSplitView::CanLightDismiss()
{
    return m_isPaneOpen && !m_isPaneClosingByLightDismiss && IsLightDismissible();
}

double CSplitView::GetOpenPaneLength()
{
    CValue value;
    double openPaneLength = 0.0;

    if (SUCCEEDED(GetValueByIndex(KnownPropertyIndex::SplitView_OpenPaneLength, &value)))
    {
        openPaneLength = value.AsDouble();

        // Support Auto/NaN for open pane length to size to the pane content.
        if (DirectUI::DoubleUtil::IsNaN(openPaneLength))
        {
            openPaneLength = m_paneMeasuredLength;
        }
    }
    else
    {
        ASSERT(false);
    }

    return openPaneLength;
}

_Check_return_ HRESULT
CSplitView::TryCloseLightDismissiblePane()
{
    ASSERT(CanLightDismiss());

    auto core = GetContext();
    xref_ptr<CSplitViewPaneClosingEventArgs> spArgs;
    spArgs.init(new CSplitViewPaneClosingEventArgs);

    // Raise the closing event to give the app a chance to cancel.
    CEventManager *pEventManager = core->GetEventManager();
    if (pEventManager != nullptr)
    {
        pEventManager->Raise(
            EventHandle(KnownEventIndex::SplitView_PaneClosing),
            TRUE /*bRefire*/,
            this,
            spArgs,
            FALSE /*fRaiseSync*/
            );
    }

    // Queue up a deferred UI thread executor that will actually close the pane
    // based on whether it was canceled or not.
    xref_ptr<CSplitViewPaneClosingExecutor> spSplitViewExecutor;
    spSplitViewExecutor.init(new CSplitViewPaneClosingExecutor(this, spArgs.get()));
    IFC_RETURN(core->ExecuteOnUIThread(spSplitViewExecutor, ReentrancyBehavior::CrashOnReentrancy));

    // Flag that we're attempting to close so that we don't queue up multiple of these messages.
    m_isPaneClosingByLightDismiss = true;

    return S_OK;
}

void CSplitView::OnCancelClosing()
{
    m_isPaneClosingByLightDismiss = false;
}

CDependencyObject*
CSplitView::ProcessTabStop(
    _In_ bool isForward,
    _In_opt_ CDependencyObject* pFocusedElement,
    _In_opt_ CDependencyObject* pCandidateTabStopElement
    )
{
    CDependencyObject* newTabStop = nullptr;

    // Panes that can be be light dismissed hold onto focus until they're closed.
    if (CanLightDismiss() && m_spPaneRoot)
    {
        if (pFocusedElement != nullptr)
        {
            // If the element losing focus is in our pane, then we evaluate the candidate element to
            // determine whether we need to override it.
            const bool isFocusedElementInPane = m_spPaneRoot->IsAncestorOf(pFocusedElement);
            if (isFocusedElementInPane)
            {
                bool doOverrideCandidate = false;

                if (pCandidateTabStopElement != nullptr)
                {
                    // If the candidate element isn't in the pane, we need to override it to keep focus within the pane.
                    doOverrideCandidate = m_spPaneRoot->IsAncestorOf(pCandidateTabStopElement) == false;
                }
                else
                {
                    // If there's no candidate, then we need to make sure focus stays within the pane.
                    doOverrideCandidate = true;
                }

                if (doOverrideCandidate)
                {
                    auto pFocusManager = VisualTree::GetFocusManagerForElement(m_spPaneRoot);
                    if (pFocusManager != nullptr)
                    {
                        auto pNewTabStop = static_cast<CDependencyObject*>(
                            isForward ? pFocusManager->GetFirstFocusableElement(m_spPaneRoot) : pFocusManager->GetLastFocusableElement(m_spPaneRoot));

                        if (pNewTabStop)
                        {
                            AddRefInterface(pNewTabStop);
                            newTabStop = pNewTabStop;
                        }
                    }
                }
            }
        }
    }

    return newTabStop;
}

void CSplitView::GetFirstFocusableElementFromPane(_Outptr_ CDependencyObject** firstFocusable)
{
    *firstFocusable = nullptr;

    if (m_spPaneRoot)
    {
        auto focusManager = VisualTree::GetFocusManagerForElement(m_spPaneRoot);
        if (focusManager != nullptr)
        {
            auto element = focusManager->GetFirstFocusableElement(m_spPaneRoot);
            if (element != nullptr)
            {
                AddRefInterface(element);
                *firstFocusable = element;
            }
        }
    }
}

void CSplitView::GetLastFocusableElementFromPane(_Outptr_ CDependencyObject** lastFocusable)
{
    *lastFocusable = nullptr;

    if (m_spPaneRoot)
    {
        auto focusManager = VisualTree::GetFocusManagerForElement(m_spPaneRoot);
        if (focusManager != nullptr)
        {
            auto element = focusManager->GetLastFocusableElement(m_spPaneRoot);
            if (element != nullptr)
            {
                AddRefInterface(element);
                *lastFocusable = element;
            }
        }
    }
}

_Check_return_ HRESULT
CSplitView::UpdateTemplateSettings()
{
    CValue value;
    double openPaneLength = GetOpenPaneLength();
    double compactLength = 0.0;
    XGRIDLENGTH gridLength;
    gridLength.type = DirectUI::GridUnitType::Pixel;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::SplitView_CompactPaneLength, &value));
    compactLength = value.AsDouble();

    // Set the template settings values.
    value.SetDouble(openPaneLength);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_OpenPaneLength, value));

    value.SetDouble(openPaneLength * -1);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_NegativeOpenPaneLength, value));

    value.SetDouble(openPaneLength - compactLength);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_OpenPaneLengthMinusCompactLength, value));

    value.SetDouble(compactLength - openPaneLength);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_NegativeOpenPaneLengthMinusCompactLength, value));

    gridLength.value = static_cast<XFLOAT>(openPaneLength);
    value.WrapGridLength(&gridLength);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_OpenPaneGridLength, value));

    gridLength.value = static_cast<XFLOAT>(compactLength);
    value.WrapGridLength(&gridLength);
    IFC_RETURN(m_pTemplateSettings->SetValueByKnownIndex(KnownPropertyIndex::SplitViewTemplateSettings_CompactPaneGridLength, value));

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::RegisterEventHandlers()
{
    CValue handler;

    handler.SetInternalHandler(OnKeyDown);
    IFC_RETURN(AddEventListener(
        EventHandle(KnownEventIndex::UIElement_KeyDown),
        &handler,
        EVENTLISTENER_INTERNAL,
        nullptr)
        );

    if (m_spLightDismissLayer)
    {
        handler.SetInternalHandler(OnLightDismissLayerPointerReleased);
        IFC_RETURN(m_spLightDismissLayer->AddEventListener(
            EventHandle(KnownEventIndex::UIElement_PointerReleased),
            &handler,
            EVENTLISTENER_INTERNAL,
            nullptr)
            );
    }

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::UnregisterEventHandlers()
{
    CValue handler;

    handler.SetInternalHandler(OnKeyDown);
    IFC_RETURN(RemoveEventListener(EventHandle(KnownEventIndex::UIElement_KeyUp), &handler));

    if (m_spLightDismissLayer)
    {
        handler.SetInternalHandler(OnLightDismissLayerPointerReleased);
        IFC_RETURN(m_spLightDismissLayer->RemoveEventListener(EventHandle(KnownEventIndex::UIElement_PointerReleased), &handler));
    }

    return S_OK;
}

_Check_return_ HRESULT CSplitView::OnPaneOpening()
{
    // Try to focus the pane if it's light-dismissible.
    if (IsLightDismissible() && m_spPaneRoot)
    {
        IFC_RETURN(SetFocusToPane());
    }

    return S_OK;
}

_Check_return_ HRESULT CSplitView::OnPaneClosing()
{
    // If the closing flag isn't set, then we're not closing due to some light-dismissible
    // action but rather are closing because the app explicitly set IsPaneOpen = false.
    // In this case, we haven't fired the PaneClosing event yet, so do it now before we
    // fire the PaneClosed event.  Note, this closing action is not cancelable, so we
    // don't care if the app sets the Cancel property on the closing event args.
    if (!m_isPaneClosingByLightDismiss)
    {
        xref_ptr<CSplitViewPaneClosingEventArgs> spArgs;
        spArgs.init(new CSplitViewPaneClosingEventArgs);

        // Raise the closing event to give the app a chance to cancel.
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager != nullptr)
        {
            pEventManager->Raise(
                EventHandle(KnownEventIndex::SplitView_PaneClosing),
                TRUE /*bRefire*/,
                this /*pSender*/,
                spArgs,
                FALSE /*fRaiseSync*/
            );
        }
    }

    if (IsLightDismissible())
    {
        IFC_RETURN(RestoreSavedFocusElement());
    }

    return S_OK;
}

_Check_return_ HRESULT CSplitView::OnPaneClosed()
{
    m_isPaneClosingByLightDismiss = false;

    // Fire the closed event.
    CEventManager *pEventManager = GetContext()->GetEventManager();
    if (pEventManager != nullptr)
    {
        pEventManager->Raise(
            EventHandle(KnownEventIndex::SplitView_PaneClosed),
            TRUE /*bRefire*/,
            this /*pSender*/,
            nullptr /*pArgs*/,
            FALSE /*fRaiseSync*/
            );
    }

    return S_OK;
}

_Check_return_ HRESULT CSplitView::SetFocusToPane()
{
    // Store weak reference to the previously focused element.
    auto pFocusManager = VisualTree::GetFocusManagerForElement(this);
    if (pFocusManager != nullptr)
    {
        auto pPrevFocusedElement = static_cast<CDependencyObject*>(pFocusManager->GetFocusedElementNoRef());
        if (pPrevFocusedElement != nullptr)
        {
            m_spPrevFocusedElementWeakRef = xref::get_weakref(pPrevFocusedElement);
            m_prevFocusState = pFocusManager->GetRealFocusStateForFocusedElement();
        }
    }

    if (m_prevFocusState == DirectUI::FocusState::Unfocused)
    {
        // We will give the pane focus using the same focus state as that of the currently focused element
        // If there is no currently focused element we will fall back to Programmatic focus state.
        m_prevFocusState = DirectUI::FocusState::Programmatic;
    }

    // Put focus on the pane.
    if (m_spPaneRoot)
    {
        ASSERT(m_prevFocusState != DirectUI::FocusState::Unfocused);
        // We'll use the previous focus state when setting focus to the pane.
        bool wasFocused = false;
        IFC_RETURN(m_spPaneRoot->Focus(m_prevFocusState, false /*animateIfBringIntoView*/, &wasFocused));
    }

    return S_OK;
}

_Check_return_ HRESULT CSplitView::RestoreSavedFocusElement()
{
    if (m_spPrevFocusedElementWeakRef)
    {
        bool wasFocusRestored = false;
        auto pPrevFocusedElement = m_spPrevFocusedElementWeakRef.lock_noref();

        // Restore focus to our cached element.
        if (pPrevFocusedElement != nullptr)
        {
            auto pFocusManager = VisualTree::GetFocusManagerForElement(pPrevFocusedElement);
            if (pFocusManager && pFocusManager->IsFocusable(pPrevFocusedElement))
            {
                const FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(pPrevFocusedElement, DirectUI::FocusNavigationDirection::None, m_prevFocusState));
                IFC_RETURN(result.GetHResult());
                wasFocusRestored = result.WasMoved();
            }
        }

        // If we failed to restore focus, then try to focus an item in the content area.
        if (!wasFocusRestored && m_spContentRoot)
        {
            bool wasFocused = false;
            IFC_RETURN(m_spContentRoot->Focus(m_prevFocusState, false /*animateIfBringIntoView*/, &wasFocused));
            // TODO: What do do if it fails?  Bug 9810211
        }

        // Reset our saved focus information.
        m_spPrevFocusedElementWeakRef.reset();
        m_prevFocusState = DirectUI::FocusState::Unfocused;
    }
    return S_OK;
}

_Check_return_ HRESULT
CSplitView::OnKeyDown(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    auto splitView = static_cast<CSplitView*>(sender);
    auto keyEventArgs = static_cast<CKeyEventArgs*>(eventArgs);
    auto pFocusManager = VisualTree::GetFocusManagerForElement(splitView);

    //Only consume the Back Key/Trap the focus within the pane
    //If Pane is open and the Display mode is either Overlay or CompatOverlay
    //For Inline modes, Auto-focus already handles the key handling because the sub-tree
    //participates in layout.
    if (pFocusManager && splitView->CanLightDismiss())
    {
        CDependencyObject* currentFocusedElement = pFocusManager->GetFocusedElementNoRef();
        CDependencyObject* autoFocusCandidate = nullptr;

        Focus::XYFocusOptions xyFocusOptions;

        DirectUI::FocusNavigationDirection navigationDirection = DirectUI::FocusNavigationDirection::None;
        switch (keyEventArgs->m_originalKeyCode)
        {
            case VK_ESCAPE:
            case VK_GAMEPAD_B:
            {
                keyEventArgs->m_bHandled = true;
                IFC_RETURN(splitView->TryCloseLightDismissiblePane());
                break;
            }
            case VK_GAMEPAD_DPAD_LEFT:
            case VK_GAMEPAD_LEFT_THUMBSTICK_LEFT:
            {
                keyEventArgs->m_bHandled = true;
                navigationDirection = DirectUI::FocusNavigationDirection::Left;
                autoFocusCandidate = pFocusManager->FindNextFocus(navigationDirection, xyFocusOptions);
                break;
            }
            case VK_GAMEPAD_DPAD_RIGHT:
            case VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT:
            {
                keyEventArgs->m_bHandled = true;
                navigationDirection = DirectUI::FocusNavigationDirection::Right;
                autoFocusCandidate = pFocusManager->FindNextFocus(navigationDirection, xyFocusOptions);
                break;
            }
            case VK_GAMEPAD_DPAD_UP:
            case VK_GAMEPAD_LEFT_THUMBSTICK_UP:
            {
                keyEventArgs->m_bHandled = true;
                navigationDirection = DirectUI::FocusNavigationDirection::Up;
                autoFocusCandidate = pFocusManager->FindNextFocus(navigationDirection, xyFocusOptions);
                break;
            }
            case VK_GAMEPAD_DPAD_DOWN:
            case VK_GAMEPAD_LEFT_THUMBSTICK_DOWN:
            {
                keyEventArgs->m_bHandled = true;
                navigationDirection = DirectUI::FocusNavigationDirection::Down;
                autoFocusCandidate = pFocusManager->FindNextFocus(navigationDirection, xyFocusOptions);
                break;
            }
        }

        if (currentFocusedElement && autoFocusCandidate)
        {
            const bool isFocusedElementInPane = splitView->m_spPaneRoot->IsAncestorOf(currentFocusedElement);

            if (isFocusedElementInPane)
            {
                bool isCandidateElementInPane = splitView->m_spPaneRoot->IsAncestorOf(autoFocusCandidate);

                //Only set focus to an auto-focus candidate if it lives in m_spPaneRoot sub-tree, swallow the input otherwise
                if (isCandidateElementInPane)
                {
                    const FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(autoFocusCandidate, navigationDirection, DirectUI::FocusState::Keyboard));
                    IFC_RETURN(result.GetHResult());
                    keyEventArgs->m_bHandled = result.WasMoved();
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSplitView::OnLightDismissLayerPointerReleased(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    auto splitView = do_pointer_cast<CSplitView>(sender->GetTemplatedParent());

    if (splitView != nullptr && splitView->CanLightDismiss())
    {
        IFC_RETURN(splitView->TryCloseLightDismissiblePane());

        auto routedEventArgs = static_cast<CRoutedEventArgs*>(eventArgs);
        routedEventArgs->m_bHandled = TRUE;
    }

    return S_OK;
}

// Content and Pane are labeled as non-visual-tree properties, meaning they won't get Enter/Leave walks by the property
// system when set in sparse storage. This is intentional, since the property system uses live Enter/Leave, which we want
// to avoid because the one live Enter/Leave walk should happen through the visual child collection. We do want non-live
// Enter/Leave walks though, so override Enter/Leave and kick those off manually.
_Check_return_ HRESULT
CSplitView::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    if (!params.fSkipNameRegistration)
    {
        EnterParams newParams(params);
        newParams.fIsLive = FALSE;

        if (m_spPaneRoot)
        {
            IFC_RETURN(m_spPaneRoot->Enter(pNamescopeOwner, newParams));
        }

        if (m_spContentRoot)
        {
            IFC_RETURN(m_spContentRoot->Enter(pNamescopeOwner, newParams));
        }
    }

    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

    return S_OK;
}

// Content and Pane are labeled as non-visual-tree properties, meaning they won't get Enter/Leave walks by the property
// system when set in sparse storage. This is intentional, since the property system uses live Enter/Leave, which we want
// to avoid because the one live Enter/Leave walk should happen through the visual child collection. We do want non-live
// Enter/Leave walks though, so override Enter/Leave and kick those off manually.
_Check_return_ HRESULT
CSplitView::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    if (!params.fSkipNameRegistration)
    {
        LeaveParams newParams(params);
        newParams.fIsLive = FALSE;

        if (m_spPaneRoot)
        {
            IFC_RETURN(m_spPaneRoot->Leave(pNamescopeOwner, newParams));
        }

        if (m_spContentRoot)
        {
            IFC_RETURN(m_spContentRoot->Leave(pNamescopeOwner, newParams));
        }
    }

    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

// Event Args
_Check_return_ HRESULT CSplitViewPaneClosingEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateSplitViewPaneClosingEventArgs(this, ppPeer));
}
