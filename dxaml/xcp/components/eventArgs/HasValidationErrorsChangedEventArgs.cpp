// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "HasValidationErrorsChangedEventArgs.h"
#include "DependencyObjectTraits.h"

CHasValidationErrorsChangedEventArgs::CHasValidationErrorsChangedEventArgs(bool newValue)
    : m_newValue(newValue)
{
}

KnownTypeIndex CHasValidationErrorsChangedEventArgs::GetTypeIndex() const
{
    return DependencyObjectTraits<CHasValidationErrorsChangedEventArgs>::Index;
}

_Check_return_ HRESULT CHasValidationErrorsChangedEventArgs::get_NewValue(_Out_ BOOLEAN* newValue)
{
    *newValue = m_newValue;
    return S_OK;
}

_Check_return_ HRESULT CHasValidationErrorsChangedEventArgs::put_NewValue(BOOLEAN newValue)
{
    m_newValue = newValue;
    return S_OK;
}

_Check_return_ HRESULT CHasValidationErrorsChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** peer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateInputValidationErrorEventArgs(this, peer));
    return S_OK;
}