// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"

_Check_return_ HRESULT CCharacterReceivedRoutedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateCharacterReceivedRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT 
CCharacterReceivedRoutedEventArgs::get_Character(
    _Out_ WCHAR* pKeyCode)
{
    *pKeyCode = static_cast<WCHAR>(m_platformKeyCode);
    return S_OK;
}

