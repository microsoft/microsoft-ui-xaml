// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "INPCListenerBase.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;
using namespace xaml_markup;

INPCListenerBase::INPCListenerBase()
    : m_propertyNameLength(0)
{ }

INPCListenerBase::~INPCListenerBase()
{ }

_Check_return_ 
HRESULT 
INPCListenerBase::AddPropertyChangedHandler(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;

    IFCEXPECT(!m_epPropertyChangedHandler);

    IFC(UpdatePropertyChangedHandler(NULL, pSource));

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT
INPCListenerBase::UpdatePropertyChangedHandler(_In_opt_ IInspectable *pOldSource, _In_opt_ IInspectable *pNewSource)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml_data::INotifyPropertyChanged> spINPC;

    const wchar_t* buffer = this->GetPropertyName();
    IFCEXPECT(buffer != nullptr);
    m_propertyNameLength = wcslen(buffer);

    if (pOldSource)
    {
        IFC(DisconnectPropertyChangedHandler(pOldSource));
    }

    if (pNewSource)
    {
        if ((spINPC = ctl::query_interface_cast<xaml_data::INotifyPropertyChanged>(pNewSource)))
        {
            IFC(m_epPropertyChangedHandler.AttachEventHandler(spINPC.Get(), 
                [this](IInspectable*, xaml_data::IPropertyChangedEventArgs *pArgs)
                {
                    return OnPropertyChangedCallback(pArgs);
                }));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
INPCListenerBase::DisconnectPropertyChangedHandler(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;

    if (m_epPropertyChangedHandler)
    {
        IFC(m_epPropertyChangedHandler.DetachEventHandler(pSource));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
INPCListenerBase::OnPropertyChangedCallback(_In_ xaml_data::IPropertyChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN propertyChanged = FALSE;
    wrl_wrappers::HString strProperty;

    IFC(pArgs->get_PropertyName(strProperty.GetAddressOf()));

    // If it is not then we need to compare the string vs. the string in the listener to see if the
    // change affects the listener
    if (strProperty.Get() != NULL)
    {
        UINT32 length = 0;
        const wchar_t* buffer = this->GetPropertyName();
        const wchar_t* current = strProperty.GetRawBuffer(&length);
        propertyChanged = (length == m_propertyNameLength) && (wcsncmp(current, buffer, length) == 0);
    }
    else 
    {
        // If the property name is NULL, which means empty, then we want the change
        propertyChanged = TRUE;
    }

    // Notify the class if the property we're listening to changed 
    if (propertyChanged)
    {
        IFC(OnPropertyChanged());
    }

Cleanup:
    RRETURN(hr);    
}


