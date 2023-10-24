// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RangeBaseAutomationPeer.g.h"
#include "RangeBase.g.h"
#include "AutomationProperties.h"
#include "PatternIdentifiers.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT RangeBaseAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IRangeBase* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IRangeBaseAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRangeBaseAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<RangeBase*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<RangeBaseAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the RangeBaseAutomationPeer class.
RangeBaseAutomationPeer::RangeBaseAutomationPeer() :
    m_isEnableValueChangedEventThrottling(false)
{
}

// Deconstructor
RangeBaseAutomationPeer::~RangeBaseAutomationPeer()
{
}

IFACEMETHODIMP RangeBaseAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_RangeValue)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(RangeBaseAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::SetValueImpl(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    DOUBLE minValue;
    DOUBLE maxValue;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<RangeBase*>(pOwner))->get_Minimum(&minValue));
    IFC((static_cast<RangeBase*>(pOwner))->get_Maximum(&maxValue));
    if(value < minValue || value > maxValue)
    {
        IFC(E_INVALIDARG);
    }

    IFC((static_cast<RangeBase*>(pOwner))->put_Value(value));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_ValueImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RangeBase*>(pOwner))->get_Value(pValue));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_IsReadOnlyImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFC(IsEnabledCore(pValue));
    *pValue = !(*pValue);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_MaximumImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RangeBase*>(pOwner))->get_Maximum(pValue));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_MinimumImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RangeBase*>(pOwner))->get_Minimum(pValue));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_LargeChangeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RangeBase*>(pOwner))->get_LargeChange(pValue));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::get_SmallChangeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RangeBase*>(pOwner))->get_SmallChange(pValue));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::RaiseMinimumPropertyChangedEvent(
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;
    IActivationFactory* pActivationFactory = NULL;
    xaml_automation::IRangeValuePatternIdentifiersStatics* pRangeValuePatternIdentifiersStatics = NULL;
    xaml_automation::IAutomationProperty* pAutomationProperty = NULL;
    BOOLEAN bAutomationListener = FALSE;
    
    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if(bAutomationListener)
    {
        pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::RangeValuePatternIdentifiers>::CreateActivationFactory();
        IFC(ctl::do_query_interface(pRangeValuePatternIdentifiersStatics, pActivationFactory));
        IFC(pRangeValuePatternIdentifiersStatics->get_MinimumProperty(&pAutomationProperty));
        IFC(RaisePropertyChangedEvent(pAutomationProperty, oldValue, newValue));
    }

Cleanup:
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(pRangeValuePatternIdentifiersStatics);
    ReleaseInterface(pAutomationProperty);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::RaiseMaximumPropertyChangedEvent(
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;
    IActivationFactory* pActivationFactory = NULL;
    xaml_automation::IRangeValuePatternIdentifiersStatics* pRangeValuePatternIdentifiersStatics = NULL;
    xaml_automation::IAutomationProperty* pAutomationProperty = NULL;
    BOOLEAN bAutomationListener = FALSE;
    
    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if(bAutomationListener)
    {
        pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::RangeValuePatternIdentifiers>::CreateActivationFactory();
        IFC(ctl::do_query_interface(pRangeValuePatternIdentifiersStatics, pActivationFactory));
        IFC(pRangeValuePatternIdentifiersStatics->get_MaximumProperty(&pAutomationProperty));
        IFC(RaisePropertyChangedEvent(pAutomationProperty, oldValue, newValue));
    }

Cleanup:
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(pRangeValuePatternIdentifiersStatics);
    ReleaseInterface(pAutomationProperty);
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBaseAutomationPeer::EnableValueChangedEventThrottling(_In_ bool value)
{
    // This flag is enabled only in MediaTransportControls for timer based change events while playing video.
    m_isEnableValueChangedEventThrottling = value;
    return S_OK;
}

_Check_return_ HRESULT RangeBaseAutomationPeer::RaiseValuePropertyChangedEvent(
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    bool shouldRaiseEvent = true;

    if (m_isEnableValueChangedEventThrottling)
    {
        // In the case progress slider in MediaTransportControls for a playing video, Value property is constantly changing causing slutter in Narration 
        // we want to avoid raising so many PropertyChangedEvents that Narrator cannot keep up.
        // We always wait for a timeout before we allow another PropertyChangedEvent to get raised.
        // This flag is enabled only in MediaTransportControls for timer based change events while playing video.
        const unsigned int timeOutInMilliseconds = 5000;
        auto now = Jupiter::HighResolutionClock::now();
        shouldRaiseEvent = (now - m_timePointOfLastValuePropertyChangedEvent) > std::chrono::milliseconds(timeOutInMilliseconds);
        if (shouldRaiseEvent)
        {
            m_timePointOfLastValuePropertyChangedEvent = now;
        }
    }
    
    BOOLEAN bAutomationListener = FALSE;
    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if (bAutomationListener && shouldRaiseEvent)
    {
        wrl::ComPtr<IActivationFactory> spActivationFactory;
        spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::RangeValuePatternIdentifiers>::CreateActivationFactory());

        wrl::ComPtr<xaml_automation::IRangeValuePatternIdentifiersStatics> spRangeValuePatternIdentifiersStatics;
        IFC_RETURN(spActivationFactory.As(&spRangeValuePatternIdentifiersStatics));

        wrl::ComPtr<xaml_automation::IAutomationProperty> spAutomationProperty;
        IFC_RETURN(spRangeValuePatternIdentifiersStatics->get_ValueProperty(&spAutomationProperty));
        IFC_RETURN(RaisePropertyChangedEvent(spAutomationProperty.Get(), oldValue, newValue));
    }

    return S_OK;
}
