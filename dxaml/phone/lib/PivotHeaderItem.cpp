// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotHeaderItem.h"
#include <PivotCommon.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    const WCHAR PivotHeaderItem::s_disabledState[] = L"Disabled";
    const WCHAR PivotHeaderItem::s_selectedState[] = L"Selected";
    const WCHAR PivotHeaderItem::s_selectedPointerOverState[] = L"SelectedPointerOver";
    const WCHAR PivotHeaderItem::s_selectedPressedState[] = L"SelectedPressed";
    const WCHAR PivotHeaderItem::s_unselectedState[] = L"Unselected";
    const WCHAR PivotHeaderItem::s_unselectedPointerOverState[] = L"UnselectedPointerOver";
    const WCHAR PivotHeaderItem::s_unselectedPressedState[] = L"UnselectedPressed";
    const WCHAR PivotHeaderItem::s_unselectedLockedState[] = L"UnselectedLocked";
    const WCHAR PivotHeaderItem::s_selectionStateGroup[] = L"SelectionStates";
    const WCHAR PivotHeaderItem::s_focusedState[] = L"Focused";
    const WCHAR PivotHeaderItem::s_unfocusedState[] = L"Unfocused";

    PivotHeaderItem::PivotHeaderItem()
        : m_pManagerNoRef(nullptr)
        , m_lastDesiredSize()
        , m_shouldSubscribeToStateChange(false)
        , m_isSelected(false)
        , m_isPressed(false)
        , m_isPointerOver(false)
        , m_shouldShowFocusStateWhenSelected(false)
        , m_selectionStateGroupCached(false)
        , m_visualStateChangedToken()
    {
    }

    _Check_return_ HRESULT
    PivotHeaderItem::InitializeImpl(_In_opt_ IInspectable* pOuter)
    {
        wrl::ComPtr<::xaml_controls::IContentControlFactory> spInnerFactory;
        wrl::ComPtr<::xaml_controls::IContentControl> spDelegatingInnerInstance;
        wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
        IInspectable* aggregateOuter = pOuter ? pOuter : static_cast<xaml_primitives::IPivotHeaderItem*>(this);

        IFC_RETURN(PivotHeaderItemGenerated::InitializeImpl());

        IFC_RETURN(wf::GetActivationFactory(
               wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ContentControl).Get(),
               &spInnerFactory));

        IFC_RETURN(spInnerFactory->CreateInstance(
               aggregateOuter,
               &spNonDelegatingInnerInspectable,
               &spDelegatingInnerInstance));

        IFC_RETURN(SetComposableBasePointers(
               spNonDelegatingInnerInspectable.Get(),
               spInnerFactory.Get()));

        // Care must be taken with any initialization after this point, as the outer object is not
        // finished fully initialized and any IInspectable operations on spDelegatingInner will
        // be delegated to the outer.
        IFC_RETURN(Private::SetDefaultStyleKey(
                spNonDelegatingInnerInspectable.Get(),
                L"Microsoft.UI.Xaml.Controls.Primitives.PivotHeaderItem"));

        // Subscribe to the IsEnabledChanged event.
        {
            EventRegistrationToken eventToken = {};

            wrl::ComPtr<xaml_controls::IControl> thisAsControl;
            IFC_RETURN(spNonDelegatingInnerInspectable.As(&thisAsControl));

            IFC_RETURN(thisAsControl->add_IsEnabledChanged(
                wrl::Callback<xaml::IDependencyPropertyChangedEventHandler>(
                [this](IInspectable*, xaml::IDependencyPropertyChangedEventArgs*)
                {
                    return UpdateVisualState(true /* useTransitions */);
                }).Get(),
                &eventToken));
        }

        // Subscribe for notification when the Template property changes
        {
            wrl::ComPtr<::xaml_controls::IControlStatics> controlStatics;
            IFC_RETURN(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
                &controlStatics));

            wrl::ComPtr<xaml::IDependencyProperty> templateProperty;
            IFC_RETURN(controlStatics->get_TemplateProperty(&templateProperty));

            wrl::ComPtr<xaml::IDependencyObject> thisAsDO;
            IFC_RETURN(spNonDelegatingInnerInspectable.As(&thisAsDO));

            // No need to hang onto the token because we want to stay subscribed to the event
            // for the lifetime of the object, and DependencyObject's destructor
            // automatically unsubscribes
            long long eventRegistrationToken = 0;
            auto changeHandler = wrl::Callback<xaml::IDependencyPropertyChangedCallback>(
                this,
                &PivotHeaderItem::OnTemplatePropertyChanged);
            IFC_RETURN(thisAsDO->RegisterPropertyChangedCallback(
                templateProperty.Get(),
                changeHandler.Get(),
                &eventRegistrationToken));
        }

        return S_OK;
    }

#pragma region IControlOverrides methods
_Check_return_ HRESULT
    PivotHeaderItem::OnPointerCaptureLostImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    const bool hasStateChanged = m_isPressed || m_isPointerOver;

    m_isPressed = false;
    m_isPointerOver = false;

    if (hasStateChanged)
    {
        IFC_RETURN(UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnPointerEnteredImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    if (!m_isPointerOver)
    {
        m_isPointerOver = true;
        IFC_RETURN(UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnPointerExitedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    boolean hasStateChanged = false;

    if (m_isPressed)
    {
        m_isPressed = false;
        hasStateChanged = true;
    }

    if (m_isPointerOver)
    {
        m_isPointerOver = false;
        hasStateChanged = true;
    }

    if (hasStateChanged)
    {
        IFC_RETURN(UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnPointerPressedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    if (!m_isPressed)
    {
        m_isPressed = true;
        IFC_RETURN(UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnPointerReleasedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    if (m_isPressed)
    {
        m_isPressed = false;
        IFC_RETURN(UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnTappedImpl(_In_ xaml_input::ITappedRoutedEventArgs* e)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(e);

    if (m_pManagerNoRef)
    {
        IFC(m_pManagerNoRef->PivotHeaderItemTapped(this));
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region IFrameworkElementOverrides methods
_Check_return_ HRESULT
PivotHeaderItem::OnApplyTemplateImpl()
{
    wrl::ComPtr<xaml_controls::IControlProtected> spThisAsControlProtected;
    wrl::ComPtr<xaml::IFrameworkElement> spRootGrid;

    IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IControlProtected), &spThisAsControlProtected));

    if (m_tpSelectionStateGroup && m_visualStateChangedToken.value != 0)
    {
        IFC_RETURN(UnsubscribeFromVisualStateGroupCompleted());
    }
    m_tpSelectionStateGroup.Clear();

    IFC_RETURN(PivotHeaderItemGenerated::OnApplyTemplateImpl());

    IFC_RETURN(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spThisAsControlProtected.Get(),
        L"Grid",
        &spRootGrid));
    IFC_RETURN(SetPtrValue(m_tpRootGrid, spRootGrid.Get()));

    wrl::ComPtr<xaml::IUIElement> spSelectedPipeAsUIElement;
    IFC_RETURN(Private::AttachTemplatePart<xaml::IUIElement>(
        spThisAsControlProtected.Get(),
        L"SelectedPipe",
        &spSelectedPipeAsUIElement));

    IFC_RETURN(SetPtrValue(m_tpSelectedPipe, spSelectedPipeAsUIElement.Get()));

    IFC_RETURN(UpdateVisualState(false));

    m_selectionStateGroupCached = false;
    if (m_shouldSubscribeToStateChange)
    {
        IFC_RETURN(SubscribeToVisualStateGroupCompleted());
    }

    return S_OK;
}
#pragma endregion

_Check_return_ HRESULT
PivotHeaderItem::UpdateVisualState(_In_ bool useTransitions)
{
    wrl::ComPtr<xaml::IVisualStateManagerStatics> spVSMStatics;
    wrl::ComPtr<xaml_controls::IControl> spPhiAsControl;
    const WCHAR* stateName = nullptr;

    IFC_RETURN(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
          &spVSMStatics));

    IFC_RETURN(GetComposableBase().As(&spPhiAsControl));

    BOOLEAN isEnabled = true;
    IFC_RETURN(spPhiAsControl->get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        stateName = s_disabledState;
    }
    else if (m_isSelected)
    {
        if (m_isPressed)
        {
            stateName = s_selectedPressedState;
        }
        else if (m_isPointerOver)
        {
            stateName = s_selectedPointerOverState;
        }
        else
        {
            stateName = s_selectedState;
        }
    }
    else
    {
        bool isLocked = false;
        if (m_pManagerNoRef)
        {
            isLocked = m_pManagerNoRef->GetIsLocked();
        }

        if (isLocked)
        {
            stateName = s_unselectedLockedState;
        }
        else
        {
            if (m_isPressed)
            {
                stateName = s_unselectedPressedState;
            }
            else if (m_isPointerOver)
            {
                stateName = s_unselectedPointerOverState;
            }
            else
            {
                stateName = s_unselectedState;
            }
        }
    }

    BOOLEAN bIgnored = FALSE;
    IFC_RETURN(spVSMStatics->GoToState(
            spPhiAsControl.Get(),
            wrl_wrappers::HStringReference(stateName).Get(),
            useTransitions,
            &bIgnored));

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::StoreDesiredSize(_Out_ BOOLEAN* pHasChanged)
{
    HRESULT hr = S_OK;

    *pHasChanged = FALSE;
    wrl::ComPtr<xaml::IUIElement> spThisAsUIE;
    wf::Size desiredSize = {};

    IFC(QueryInterface(
        __uuidof(xaml::IUIElement), &spThisAsUIE));

    IFC(spThisAsUIE->get_DesiredSize(&desiredSize));

    *pHasChanged = (desiredSize.Width != m_lastDesiredSize.Width ||
        desiredSize.Height != m_lastDesiredSize.Height);
    m_lastDesiredSize = desiredSize;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderItem::SetHeaderManagerCallbacks(_In_ xaml_controls::IPivotHeaderManagerItemEvents* pHeaderManager)
{
    m_pManagerNoRef = pHeaderManager;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PivotHeaderItem::SetSubscribeToStateChangeCallback(_In_ bool shouldSubscribe)
{
    if (m_shouldSubscribeToStateChange != shouldSubscribe)
    {
        m_shouldSubscribeToStateChange = shouldSubscribe;

        if (shouldSubscribe)
        {
            IFC_RETURN(SubscribeToVisualStateGroupCompleted());
        }
        else
        {
            IFC_RETURN(UnsubscribeFromVisualStateGroupCompleted());
        }
    }

    return S_OK;
}

void
PivotHeaderItem::ClearIsHovered()
{
    m_isPointerOver = false;
}

void
PivotHeaderItem::SetIsSelected(_In_ bool isSelected)
{
    m_isSelected = isSelected;
}

void
PivotHeaderItem::SetShouldShowFocusWhenSelected(_In_ bool shouldShowFocusStateWhenSelected)
{
    m_shouldShowFocusStateWhenSelected = shouldShowFocusStateWhenSelected;
}

_Check_return_ HRESULT
PivotHeaderItem::SubscribeToVisualStateGroupCompleted()
{
    if (m_visualStateChangedToken.value == 0)
    {
        IFC_RETURN(EnsureSelectionStateGroup());

        if (m_tpSelectionStateGroup)
        {
            IFC_RETURN(m_tpSelectionStateGroup->add_CurrentStateChanged(
                wrl::Callback<xaml::IVisualStateChangedEventHandler>(
                this, &PivotHeaderItem::OnVisualStateChanged).Get(), &m_visualStateChangedToken));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::UnsubscribeFromVisualStateGroupCompleted()
{
    if (m_visualStateChangedToken.value != 0)
    {
        // If we subscribed to the CurrentStateChanged, it means there is a selection state group.
        // In case we get retemplated, OnApplyTemplate calls this method before hooking up the new parts.
        ASSERT(m_tpSelectionStateGroup);
        IFC_RETURN(m_tpSelectionStateGroup->remove_CurrentStateChanged(m_visualStateChangedToken));
        m_visualStateChangedToken.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnVisualStateChanged(_In_ IInspectable* pSender, _In_ xaml::IVisualStateChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pSender);

    wrl::ComPtr<xaml::IVisualState> spNewState;
    wrl_wrappers::HString strStateName;

    IFC(pArgs->get_NewState(&spNewState));

    // PivotHeaderItem may be in the NULL VisualState if it was retemplated with VisualStateTriggers
    // and a context changed occurred where no StateTriggers evaluated to true.
    if (spNewState)
    {
        IFC(spNewState->get_Name(strStateName.GetAddressOf()));

        // We signal when we've successfully transitioned out of the locked
        // state to the HeaderManager.
        if ((wrl_wrappers::HStringReference(s_unselectedState) == strStateName ||
            wrl_wrappers::HStringReference(s_selectedState) == strStateName) && m_pManagerNoRef)
        {
            IFC(m_pManagerNoRef->VsmUnlockedStateChangeCompleteEvent());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderItem::EnsureSelectionStateGroup()
{
    if (!m_selectionStateGroupCached)
    {
        wrl::ComPtr<xaml::IVisualStateGroup> spSelectionStateGroup;
        IFC_RETURN(FindVisualStateGroup(wrl_wrappers::HStringReference(s_selectionStateGroup).Get(), &spSelectionStateGroup));

        if (spSelectionStateGroup)
        {
            IFC_RETURN(SetPtrValue(m_tpSelectionStateGroup, spSelectionStateGroup.Get()));
            m_selectionStateGroupCached = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::FindVisualStateGroup(_In_ HSTRING hName, _Outptr_result_maybenull_ xaml::IVisualStateGroup** ppStateGroup)
{
    *ppStateGroup = nullptr;

    if (!m_tpRootGrid)
    {
        return S_OK;
    }

    wrl::ComPtr<xaml::IVisualStateManagerStatics> spVsmStatics;
    wrl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> spVisualStateGroupVect;

    *ppStateGroup = nullptr;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
        &spVsmStatics));

    IFC_RETURN(spVsmStatics->GetVisualStateGroups(m_tpRootGrid.Get(), &spVisualStateGroupVect));

    if (spVisualStateGroupVect)
    {
        UINT groupVectSize = 0;

        IFC_RETURN(spVisualStateGroupVect->get_Size(&groupVectSize));

        for (UINT idx = 0; idx < groupVectSize; idx++)
        {
            wrl_wrappers::HString strGroupName;
            wrl::ComPtr<xaml::IVisualStateGroup> spGroup;
            IFC_RETURN(spVisualStateGroupVect->GetAt(idx, &spGroup));

            IFC_RETURN(spGroup->get_Name(strGroupName.ReleaseAndGetAddressOf()));

            if (hName == strGroupName)
            {
                *ppStateGroup = spGroup.Detach();
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderItem::OnTemplatePropertyChanged(xaml::IDependencyObject*, xaml::IDependencyProperty* )
{
    // Previously cached template parts are invalid when the Template changes
    m_tpRootGrid.Clear();
    m_tpSelectedPipe.Clear();

    if (m_tpSelectionStateGroup && m_visualStateChangedToken.value != 0)
    {
        IFC_RETURN(UnsubscribeFromVisualStateGroupCompleted());
    }

    m_tpSelectionStateGroup.Clear();
    m_selectionStateGroupCached = false;

    return S_OK;
}

} } } } } XAML_ABI_NAMESPACE_END
