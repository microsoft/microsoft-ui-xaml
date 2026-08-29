// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "ValidationErrorEventArgs.h"
#include "DependencyObjectTraits.h"

CInputValidationErrorEventArgs::CInputValidationErrorEventArgs(
    const DirectUI::InputValidationErrorEventAction action,
    _In_ xaml_controls::IInputValidationError* error)
    : m_errorAction(action)
    , m_error(error)
{
}

KnownTypeIndex CInputValidationErrorEventArgs::GetTypeIndex() const
{
    return DependencyObjectTraits<CInputValidationErrorEventArgs>::Index;
}

_Check_return_ HRESULT CInputValidationErrorEventArgs::get_Action(_Out_ DirectUI::InputValidationErrorEventAction* action)
{
    *action = m_errorAction;
    return S_OK;
}

_Check_return_ HRESULT CInputValidationErrorEventArgs::put_Action(_In_ DirectUI::InputValidationErrorEventAction action)
{
    m_errorAction = action;
    return S_OK;
}

_Check_return_ HRESULT CInputValidationErrorEventArgs::get_Error(_COM_Outptr_ IUnknown** error)
{
    IFC_RETURN(m_error.CopyTo(error));
    return S_OK;
}

_Check_return_ HRESULT CInputValidationErrorEventArgs::put_Error(_In_ IUnknown* error)
{
    IFC_RETURN(error->QueryInterface(m_error.ReleaseAndGetAddressOf()));
    return S_OK;
}

_Check_return_ HRESULT CInputValidationErrorEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** peer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateInputValidationErrorEventArgs(this, peer));
    return S_OK;
}