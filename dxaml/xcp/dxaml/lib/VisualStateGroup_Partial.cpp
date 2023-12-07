// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualStateGroup.g.h"
#include "VisualStateChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT VisualStateGroup::get_NameImpl(
    _Out_ HSTRING* phName)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spName;

    IFCPTR(phName);

    IFC(GetValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name),
        &spName));

    IFC(ctl::do_get_value(*phName, spName.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VisualStateGroup::get_CurrentStateImpl(
    _Outptr_ xaml::IVisualState** pValue)
{
    HRESULT hr = S_OK;
    CValue value;
    CDependencyObject* currentState = NULL;

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::VisualStateGroup_GetCurrentState(
        static_cast<CVisualStateGroup*>(GetHandle()),
        &value));

    currentState = value.AsObject();
    if (currentState)
    {
        ctl::ComPtr<DependencyObject> spCurrentStateAsDO;

        IFC(DXamlCore::GetCurrent()->GetPeer(
            currentState,
            &spCurrentStateAsDO));
        IFC(spCurrentStateAsDO.CopyTo(pValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualStateGroup::GetState(
    _In_ HSTRING hStateName,
    _Outptr_ xaml::IVisualState** ppState)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::VisualState*>> spStates;
    ctl::ComPtr<wfc::IIterable<xaml::VisualState*>> spIterable;
    ctl::ComPtr<wfc::IIterator<xaml::VisualState*>> spIterator;
    BOOLEAN bHasCurrent;
    wrl_wrappers::HString strCurrentName;

    IFCPTR(ppState);
    *ppState = NULL;

    IFC(get_States(&spStates));
    IFCPTR(spStates.Get());

    IFC(spStates.As(&spIterable));
    IFC(spIterable->First(&spIterator));
    IFC(spIterator->get_HasCurrent(&bHasCurrent));

    while (bHasCurrent)
    {
        ctl::ComPtr<IVisualState> spState;

        IFC(spIterator->get_Current(&spState));
        IFC(spState->get_Name(strCurrentName.ReleaseAndGetAddressOf()));

        if (strCurrentName == hStateName)
        {
            IFC(spState.CopyTo(ppState));
            goto Cleanup;
        }

        IFC(spIterator->MoveNext(&bHasCurrent));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualStateGroup::RaiseCurrentStateChanging(
    _In_ xaml::IFrameworkElement* pElement,
    _In_ xaml::IVisualState* pOldState,
    _In_ xaml::IVisualState* pNewState,
    _In_ xaml_controls::IControl* pControl)
{
    HRESULT hr = S_OK;
    VisualStateGroup::CurrentStateChangingEventSourceType* pChangingEventSource = nullptr;
    ctl::ComPtr<VisualStateChangedEventArgs> spArgs;

    IFC(ctl::make(&spArgs));
    IFC(spArgs->put_OldState(pOldState));
    IFC(spArgs->put_NewState(pNewState));
    IFC(spArgs->put_Control(pControl));

    IFC(GetCurrentStateChangingEventSourceNoRef(&pChangingEventSource));
    IFC(pChangingEventSource->Raise(pElement, spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualStateGroup::RaiseCurrentStateChanged(
    _In_ xaml::IFrameworkElement* pElement,
    _In_ xaml::IVisualState* pOldState,
    _In_ xaml::IVisualState* pNewState,
    _In_ xaml_controls::IControl* pControl)
{
    HRESULT hr = S_OK;
    CurrentStateChangedEventSourceType* pChangedEventSource = nullptr;
    ctl::ComPtr<VisualStateChangedEventArgs> spArgs;

    IFC(ctl::make(&spArgs));
    IFC(spArgs->put_OldState(pOldState));
    IFC(spArgs->put_NewState(pNewState));
    IFC(spArgs->put_Control(pControl));

    IFC(GetCurrentStateChangedEventSourceNoRef(&pChangedEventSource));
    IFC(pChangedEventSource->Raise(pElement, spArgs.Get()));

Cleanup:
    RRETURN(hr);
}
