// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SemanticZoomAutomationPeer.g.h"
#include "SemanticZoom.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SemanticZoomAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ISemanticZoom* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISemanticZoomAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISemanticZoomAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<SemanticZoom*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<SemanticZoomAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the SemanticZoomAutomationPeer class.
SemanticZoomAutomationPeer::SemanticZoomAutomationPeer()
{
}

// Deconstructor
SemanticZoomAutomationPeer::~SemanticZoomAutomationPeer()
{
}

IFACEMETHODIMP SemanticZoomAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Toggle)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(SemanticZoomAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SemanticZoomAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SemanticZoom")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SemanticZoomAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_SemanticZoom;
    RRETURN(S_OK);
}

_Check_return_ HRESULT SemanticZoomAutomationPeer::ToggleImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<SemanticZoom*>(pOwner))->AutomationSemanticZoomOnToggle());

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT SemanticZoomAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isZoomedInViewActive = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<SemanticZoom*>(pOwner))->get_IsZoomedInViewActive(&isZoomedInViewActive));

    if(isZoomedInViewActive)
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_On;
    }
    else
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_Off;
    }

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT SemanticZoomAutomationPeer::RaiseToggleStatePropertyChangedEvent(_In_ BOOLEAN bNewValue)
{
    HRESULT hr = S_OK;
    xaml_automation::ToggleState oldValue = xaml_automation::ToggleState::ToggleState_On;
    xaml_automation::ToggleState newValue = xaml_automation::ToggleState::ToggleState_On;
    CValue valueOld;
    CValue valueNew;

    if(bNewValue)
    {
        oldValue = xaml_automation::ToggleState::ToggleState_Off;
    }
    else
    {
        newValue = xaml_automation::ToggleState::ToggleState_Off;
    }

    IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
    IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));

    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SemanticZoomAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<xaml::IFrameworkElement> spCurrentView;

    IFCPTR(ppReturnValue);
    IFC(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&spAPChildren));

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    IFC(spOwner.Cast<SemanticZoom>()->AutomationGetActivePresenter(&spCurrentView));

    if (spCurrentView.Get())
    {
        ctl::ComPtr<xaml::IUIElement> spCurrentViewAsUI;

        IFC(spCurrentView.As(&spCurrentViewAsUI));
        IFC(GetAutomationPeerChildren(spCurrentViewAsUI.Get(), spAPChildren.Get()));
    }

    IFC(spAPChildren.CopyTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

