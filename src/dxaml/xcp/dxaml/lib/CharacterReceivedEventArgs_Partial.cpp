// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CharacterReceivedRoutedEventArgs.g.h"
#include "RoutedEventArgs.h"
#include "CharacterReceivedEventArgs.h"

_Check_return_ HRESULT DirectUI::CharacterReceivedRoutedEventArgs::get_KeyStatusImpl(_Out_ wuc::CorePhysicalKeyStatus* pValue)
{
    PhysicalKeyStatus physicalKeyStatus;
    xref_ptr<CEventArgs> spCoreArgs;
    IFCPTR_RETURN(pValue);

    spCoreArgs.attach(GetCorePeer());
    physicalKeyStatus = static_cast<CCharacterReceivedRoutedEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->m_physicalKeyStatus;

    pValue->RepeatCount = physicalKeyStatus.m_uiRepeatCount;
    pValue->ScanCode = physicalKeyStatus.m_uiScanCode;
    pValue->IsExtendedKey = !!physicalKeyStatus.m_bIsExtendedKey;
    pValue->IsMenuKeyDown = !!physicalKeyStatus.m_bIsMenuKeyDown;
    pValue->WasKeyDown = !!physicalKeyStatus.m_bWasKeyDown;
    pValue->IsKeyReleased = !!physicalKeyStatus.m_bIsKeyReleased;

    return S_OK;
}

_Check_return_ HRESULT DirectUI::CharacterReceivedRoutedEventArgs::get_CharacterImpl(_Out_ WCHAR* pValue)
{
    xref_ptr<CEventArgs> spCoreArgs;
    IFCPTR_RETURN(pValue);
    spCoreArgs.attach(GetCorePeer());
    wsy::VirtualKey virtualKey = static_cast<CCharacterReceivedRoutedEventArgs*>(spCoreArgs.get())->m_platformKeyCode;
    ASSERT(virtualKey <= WCHAR_MAX);
    *pValue = static_cast<WCHAR>(virtualKey);
                    
    return S_OK;
}

