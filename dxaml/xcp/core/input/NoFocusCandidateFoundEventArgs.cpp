// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "NoFocusCandidateFoundEventArgs.h"

_Check_return_ HRESULT CNoFocusCandidateFoundEventArgs::get_Direction(_Out_ DirectUI::FocusNavigationDirection* focusNavigationDirection)
{
    *focusNavigationDirection = m_focusNavigationDirection;
    return S_OK;
}

_Check_return_ HRESULT CNoFocusCandidateFoundEventArgs::get_Handled(_Out_ BOOLEAN *handled)
{
    *handled = (m_bHandled == TRUE);
    return S_OK;
}

_Check_return_ HRESULT CNoFocusCandidateFoundEventArgs::put_Handled(_In_ BOOLEAN handled)
{
    m_bHandled = handled;
    return S_OK;
}

_Check_return_ HRESULT CNoFocusCandidateFoundEventArgs::get_InputDevice(_Out_ DirectUI::FocusInputDeviceKind* inputDevice)
{
    *inputDevice = m_inputDevice;
    return S_OK;
}

_Check_return_ HRESULT CNoFocusCandidateFoundEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateNoFocusCandidateFoundEventArgs(this, ppPeer));

    return S_OK;
}