// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RangeBase.g.h"
#include "RangeBaseAutomationPeer.g.h"
#include "RangeBaseValueChangedEventArgs.g.h"

using namespace DirectUI;

IFACEMETHODIMP RangeBase::put_Minimum(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    IFC(EnsureValidDoubleValue(value));
    IFC(RangeBaseGenerated::put_Minimum(value));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RangeBase::put_Maximum(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    IFC(EnsureValidDoubleValue(value));
    IFC(RangeBaseGenerated::put_Maximum(value));

Cleanup:
    return hr;
}

IFACEMETHODIMP RangeBase::put_Value(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    IFC(EnsureValidDoubleValue(value));
    IFC(RangeBaseGenerated::put_Value(value));

Cleanup:
    return hr;
}

IFACEMETHODIMP RangeBase::put_SmallChange(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    IFC(EnsureValidDoubleValue(value));
    IFC(RangeBaseGenerated::put_SmallChange(value));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RangeBase::put_LargeChange(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    IFC(EnsureValidDoubleValue(value));
    IFC(RangeBaseGenerated::put_LargeChange(value));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBase::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::RangeBase_LargeChange:
    case KnownPropertyIndex::RangeBase_Maximum:
        pValue->SetDouble(1.0);
        break;
    case KnownPropertyIndex::RangeBase_SmallChange:
        pValue->SetDouble(0.1);
        break;
    default:
        IFC(RangeBaseGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT RangeBase::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(RangeBaseGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RangeBase_Minimum:
            IFC(HandlePropertyChanged(
                args,
                &RangeBaseAutomationPeer::RaiseMinimumPropertyChangedEvent,
                &RangeBaseGenerated::OnMinimumChangedProtected));
            break;

        case KnownPropertyIndex::RangeBase_Maximum:
            IFC(HandlePropertyChanged(
                args,
                &RangeBaseAutomationPeer::RaiseMaximumPropertyChangedEvent,
                &RangeBaseGenerated::OnMaximumChangedProtected));
            break;

        case KnownPropertyIndex::RangeBase_Value:
            IFC(HandlePropertyChanged(
                args,
                &RangeBaseAutomationPeer::RaiseValuePropertyChangedEvent,
                &RangeBaseGenerated::OnValueChangedProtected));
            break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RangeBase::HandlePropertyChanged(
    _In_ const PropertyChangedParams& args,
    HRESULT (RangeBaseAutomationPeer::*OnChanged)(const CValue&, const CValue&),
    HRESULT (RangeBaseGenerated::*OnChangedProtected)(DOUBLE, DOUBLE))
{
    DOUBLE oldValue = 0.0;
    DOUBLE newValue = 0.0;
    IFC_RETURN(CValueBoxer::UnboxValue(args.m_pOldValue, &oldValue));
    IFC_RETURN(CValueBoxer::UnboxValue(args.m_pNewValue, &newValue));

    BOOLEAN bAutomationListener = FALSE;
    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));

    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> automationPeer;

        IFC_RETURN(GetOrCreateAutomationPeer(automationPeer.ReleaseAndGetAddressOf()));

        if (automationPeer)
        {
            CValue tempOldValue, tempNewValue;
            tempOldValue.SetDouble(oldValue);
            tempNewValue.SetDouble(newValue);

            ctl::ComPtr<xaml_automation_peers::IRangeBaseAutomationPeer> rangeBaseAutomationPeer;
            IFC_RETURN(automationPeer.As(&rangeBaseAutomationPeer));
            IFC_RETURN((rangeBaseAutomationPeer.Cast<RangeBaseAutomationPeer>()->*OnChanged)(tempOldValue, tempNewValue));
        }
    }

    IFC_RETURN((this->*OnChangedProtected)(oldValue, newValue));

    return S_OK;
}

// Raises the ValueChanged routed event.
_Check_return_ HRESULT RangeBase::OnValueChangedImpl(_In_ DOUBLE oldValue, _In_ DOUBLE newValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<RangeBaseValueChangedEventArgs> spArgs;

    if (ShouldRaiseEvent(static_cast<KnownEventIndex>(KnownEventIndex::RangeBase_ValueChanged)))
    {
        // Create the args
        IFC(ctl::make(&spArgs));

        IFC(spArgs->put_OldValue(oldValue));
        IFC(spArgs->put_NewValue(newValue));

        // Raise the event
        ValueChangedEventSourceType* pEventSourceNoRef = nullptr;
        IFC(GetValueChangedEventSourceNoRef(&pEventSourceNoRef));
        IFC(pEventSourceNoRef->Raise(ctl::as_iinspectable(this), spArgs.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Create RangeBaseAutomationPeer to represent the RangeBase.
IFACEMETHODIMP RangeBase::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRangeBaseAutomationPeer* pRangeBaseAutomationPeer = NULL;
    xaml_automation_peers::IRangeBaseAutomationPeerFactory* pRangeBaseAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::RangeBaseAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pRangeBaseAPFactory, pActivationFactory));

    IFC(static_cast<RangeBaseAutomationPeerFactory*>(pRangeBaseAPFactory)->CreateInstanceWithOwner(this,
        NULL,
        &inner,
        &pRangeBaseAutomationPeer));
    IFC(ctl::do_query_interface(*ppAutomationPeer, pRangeBaseAutomationPeer));

Cleanup:
    ReleaseInterface(pRangeBaseAutomationPeer);
    ReleaseInterface(pRangeBaseAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}

// Makes sure the DOUBLE value is not Double.NaN, Double.PositiveInfinity, or Double.NegativeInfinity.
// If the value passed is one of these illegal values, throws an ArgumentException with a custom message.
_Check_return_ HRESULT
RangeBase::EnsureValidDoubleValue(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    if (DoubleUtil::IsNaN(value) || DoubleUtil::IsInfinity(value))
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
    }

Cleanup:
    RRETURN(hr);
}