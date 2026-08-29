// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"

_Check_return_ HRESULT CKeyEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateKeyRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT 
CKeyEventArgs::GetKeyStatus(
    _Out_ PhysicalKeyStatus* pKeyStatus)
{
    IFCEXPECT_ASSERT_RETURN(pKeyStatus);
    
    *pKeyStatus = m_physicalKeyStatus;

    return S_OK;
}

void CKeyEventArgs::SetModifierKeys(_In_ const XUINT32 modifierKeys)
{
    IFCFAILFAST(put_Shift(modifierKeys & KEY_MODIFIER_SHIFT));
    IFCFAILFAST(put_Ctrl(modifierKeys & KEY_MODIFIER_CTRL));
    put_Alt(modifierKeys & KEY_MODIFIER_ALT);
}
