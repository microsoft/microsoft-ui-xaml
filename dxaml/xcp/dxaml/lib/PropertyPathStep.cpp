// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPathStep.h"
#include "PropertyPath.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

_Check_return_ HRESULT PropertyPathStep::Initialize(PropertyPathListener *pListener)
{
    IFC_RETURN(ctl::AsWeak(pListener, &m_spListener));
    return S_OK;
}


void PropertyPathStep::Disconnect()
{
    if (m_epCurrentChangedHandler)
    {
        VERIFYHR(m_epCurrentChangedHandler.DetachEventHandler(m_tpSourceAsCV.Get()));
    }

    m_tpSourceAsCV.Clear();

    DisconnectCurrentItem();
}

PropertyPathStep::~PropertyPathStep()
{ 
    VERIFYHR(SafeRemoveCurrentChangedEventHandler());
}

_Check_return_ 
HRESULT 
PropertyPathStep::AddCurrentChangedEventHandler()
{
    HRESULT hr = S_OK;

    ASSERT(!m_epCurrentChangedHandler);
    ASSERT(m_tpSourceAsCV.Get());

    IFC(m_epCurrentChangedHandler.AttachEventHandler(m_tpSourceAsCV.Get(),
        [this](IInspectable *sender, IInspectable *args){
            return CollectionViewCurrentChanged(); 
        }));

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathStep::SafeRemoveCurrentChangedEventHandler()
{
    HRESULT hr = S_OK;

    // if we do not have an event handler allocated
    // this means that we have nothing to do here
    if (m_epCurrentChangedHandler)
    {
        auto spSource = m_tpSourceAsCV.GetSafeReference();
        if (spSource)
        {
            IFC(m_epCurrentChangedHandler.DetachEventHandler(spSource.Get()));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathStep::RaiseSourceChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPropertyPathListener> spListener;

    IFC(m_spListener.As(&spListener));

    // If the listener is gone, no-op.
    if (spListener)
    {
        IFC(spListener.Cast<PropertyPathListener>()->PropertyPathStepChanged(this));
    }

Cleanup:

    RRETURN(hr);
}
