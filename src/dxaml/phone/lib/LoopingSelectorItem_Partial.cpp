// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

LoopingSelectorItem::LoopingSelectorItem()
    : _state(State::Normal)
    , _visualIndex(0)
    , _pParentNoRef(nullptr)
    , _hasPeerBeenCreated(FALSE)
{}

_Check_return_
HRESULT
LoopingSelectorItem::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IContentControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IContentControl> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(LoopingSelectorItemGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ContentControl).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<ILoopingSelectorItem*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.Primitives.LoopingSelectorItem"));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItem::OnPointerEnteredImpl(_In_ xaml::Input::IPointerRoutedEventArgs* pEventArgs)
{
    auto pointerDeviceType = mui::PointerDeviceType_Touch;
    wrl::ComPtr<ixp::IPointerPoint> spPointerPoint;

    IFC_RETURN(pEventArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR_RETURN(spPointerPoint);
    IFC_RETURN(spPointerPoint->get_PointerDeviceType(&pointerDeviceType));

    if (pointerDeviceType == mui::PointerDeviceType_Mouse)
    {
        if (_state != State::Selected)
        {
            IFC_RETURN(SetState(State::PointerOver, FALSE));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItem::OnPointerPressedImpl(_In_ xaml::Input::IPointerRoutedEventArgs*)
{
    if (_state != State::Selected)
    {
        IFC_RETURN(SetState(State::Pressed, FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItem::OnPointerExitedImpl(_In_ xaml::Input::IPointerRoutedEventArgs*)
{
    if (_state != State::Selected)
    {
        IFC_RETURN(SetState(State::Normal, FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItem::OnPointerCaptureLostImpl(_In_ xaml_input::IPointerRoutedEventArgs*)
{
    if (_state != State::Selected)
    {
        IFC_RETURN(SetState(State::Normal, FALSE));
    }

    return S_OK;
}

#pragma region IUIElementOverrides
_Check_return_ HRESULT
LoopingSelectorItem::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_automation_peers::LoopingSelectorItemAutomationPeer> spLoopingSelectorItemAutomationPeer;

    IFC(wrl::MakeAndInitialize<xaml_automation_peers::LoopingSelectorItemAutomationPeer>
            (&spLoopingSelectorItemAutomationPeer, this));

    _hasPeerBeenCreated = TRUE;

    IFC(spLoopingSelectorItemAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}
#pragma endregion

_Check_return_ HRESULT LoopingSelectorItem::GoToState(_In_ State newState, _In_ BOOLEAN useTransitions)
{
    LPCWSTR strState = nullptr;

    switch (newState)
    {
    case State::Normal:
        strState = L"Normal";
        break;
    case State::Expanded:
        strState = L"Expanded";
        break;
    case State::Selected:
        strState = L"Selected";
        break;
    case State::PointerOver:
        strState = L"PointerOver";
        break;
    case State::Pressed:
        strState = L"Pressed";
        break;
    }

    wrl::ComPtr<xaml::IVisualStateManagerStatics> spVSMStatics;
    wrl::ComPtr<xaml_controls::IControl> spThisAsControl;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
        &spVSMStatics));

    IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IControl), &spThisAsControl));

    boolean returnValue = false;
    IFC_RETURN(spVSMStatics->GoToState(spThisAsControl.Get(), wrl_wrappers::HStringReference(strState).Get(), useTransitions, &returnValue));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItem::SetState(_In_ State newState, _In_ BOOLEAN useTransitions)
{
    HRESULT hr = S_OK;
    // NOTE: Not calling GoToState when the LSI is already in the target
    // state allows us to keep animations looking smooth when the following
    // sequence of events happens:
    // LS starts closing -> Items changes -> LSIs are Recycled/Realized/Assigned New Content
    if (newState != _state)
    {
        IFC(GoToState(newState, useTransitions));
        _state = newState;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItem::AutomationSelect()
{
    HRESULT hr = S_OK;

    LoopingSelector* pLoopingSelectorNoRef = nullptr;
    IFC(GetParentNoRef(&pLoopingSelectorNoRef));
    IFC(pLoopingSelectorNoRef->AutomationScrollToVisualIdx(_visualIndex, true /* ignoreScrollingState */));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItem::AutomationGetSelectionContainerUIAPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppPeer)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerStatics> spAutomationPeerStatics;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    wrl::ComPtr<xaml::IUIElement> spLoopingSelectorAsUI;
    LoopingSelector* pLoopingSelectorNoRef = nullptr;

    IFC(GetParentNoRef(&pLoopingSelectorNoRef));
    IFC(pLoopingSelectorNoRef->QueryInterface(
        __uuidof(xaml::IUIElement),
        &spLoopingSelectorAsUI));

    IFC(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
          &spAutomationPeerStatics));
    IFC(spAutomationPeerStatics->CreatePeerForElement(spLoopingSelectorAsUI.Get(), &spAutomationPeer));
    IFC(spAutomationPeer.CopyTo(ppPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItem::AutomationGetIsSelected(_Out_ BOOLEAN *value)
{
    LoopingSelector* loopingSelectorNoRef = nullptr;
    INT selectedIdx;

    IFC_RETURN(GetParentNoRef(&loopingSelectorNoRef));
    IFC_RETURN(loopingSelectorNoRef->get_SelectedIndex(&selectedIdx));

    UINT32 itemIndex = 0;
    IFC_RETURN(loopingSelectorNoRef->VisualIndexToItemIndex(_visualIndex, &itemIndex));

    *value = (selectedIdx == static_cast<INT>(itemIndex));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelectorItem::AutomationUpdatePeerIfExists(_In_ int itemIndex)
{
    HRESULT hr = S_OK;

    if (_hasPeerBeenCreated)
    {
        wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
        wrl::ComPtr<xaml_automation_peers::ILoopingSelectorItemAutomationPeer> spLSIAP;
        xaml_automation_peers::LoopingSelectorItemAutomationPeer* pLSIAP;
        wrl::ComPtr<xaml::IUIElement> spThisAsUI;
        wrl::ComPtr<xaml_automation_peers::IFrameworkElementAutomationPeerStatics> spAutomationPeerStatics;

        IFC(QueryInterface(__uuidof(xaml::IUIElement), &spThisAsUI));
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
            &spAutomationPeerStatics));
        // CreatePeerForElement does not always create one - if there is one associated with this UIElement it will reuse that.
        // We do not want to end up creating a bunch of peers for the same element causing Narrator to get confused.
        IFC(spAutomationPeerStatics->CreatePeerForElement(spThisAsUI.Get(), &spAutomationPeer));
        IFC(spAutomationPeer.As(&spLSIAP));

        pLSIAP = static_cast<xaml_automation_peers::LoopingSelectorItemAutomationPeer*>(spLSIAP.Get());

        IFC(pLSIAP->UpdateEventSource());
        IFC(pLSIAP->UpdateItemIndex(itemIndex));
    }

Cleanup:
    RRETURN(hr);
}

} } } } } XAML_ABI_NAMESPACE_END
