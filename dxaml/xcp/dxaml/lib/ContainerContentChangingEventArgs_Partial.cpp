// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContainerContentChangingEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// ctor
ContainerContentChangingEventArgs::ContainerContentChangingEventArgs()
    : m_pContainerBackPointer(nullptr)
    , m_xbindingPhase(-1)
{
}

// direct path to the container, so that no cycles exist
_Check_return_ HRESULT ContainerContentChangingEventArgs::get_ItemContainerImpl(_Outptr_ xaml_primitives::ISelectorItem** pValue)
{
    HRESULT hr = S_OK;

    *pValue = ctl::query_interface<ISelectorItem>(m_pContainerBackPointer);

    RRETURN(hr);
}

// reset the args in such a way that it is not holding on to any other structures
_Check_return_ HRESULT ContainerContentChangingEventArgs::ResetLifetimeImpl()
{
    HRESULT hr = S_OK;

    IFC(put_Callback(nullptr));
    IFC(put_Item(nullptr));
Cleanup:
    RRETURN(hr);
}

// Register this container for a callback on the next phase
_Check_return_ HRESULT ContainerContentChangingEventArgs::RegisterUpdateCallbackImpl(_In_ wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>* callback)
{
    RRETURN(RegisterUpdateCallbackWithPhaseImpl(m_phase+1, callback));
}

// Register this container for a callback on a specific phase
_Check_return_ HRESULT ContainerContentChangingEventArgs::RegisterUpdateCallbackWithPhaseImpl(_In_ UINT callbackPhase, _In_ wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>* callback)
{
    HRESULT hr = S_OK;

    m_phase = callbackPhase;
    IFC(put_Callback(callback));
    IFC(put_WantsCallBack(TRUE));

Cleanup:
    RRETURN(hr);
}
