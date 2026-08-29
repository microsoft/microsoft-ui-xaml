// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyRoutedEventArgs.g.h"
#include "RoutedEventArgs.h"
#include "KeyboardEventArgs.h"

_Check_return_ HRESULT DirectUI::KeyRoutedEventArgs::get_KeyStatusImpl(
    _Out_ wuc::CorePhysicalKeyStatus* pCorePhysicalKeyStatus)
{
    PhysicalKeyStatus physicalKeyStatus;
    xref_ptr<CEventArgs> spCoreArgs;

    IFCPTR_RETURN(pCorePhysicalKeyStatus);

    spCoreArgs.attach(GetCorePeer());

    IFCEXPECT_ASSERT_RETURN(spCoreArgs);
    IFC_RETURN(static_sp_cast<CKeyEventArgs>(spCoreArgs)->GetKeyStatus(&physicalKeyStatus));

    pCorePhysicalKeyStatus->RepeatCount = physicalKeyStatus.m_uiRepeatCount;
    pCorePhysicalKeyStatus->ScanCode = physicalKeyStatus.m_uiScanCode;
    pCorePhysicalKeyStatus->IsExtendedKey = !!physicalKeyStatus.m_bIsExtendedKey;
    pCorePhysicalKeyStatus->IsMenuKeyDown = !!physicalKeyStatus.m_bIsMenuKeyDown;
    pCorePhysicalKeyStatus->WasKeyDown = !!physicalKeyStatus.m_bWasKeyDown;
    pCorePhysicalKeyStatus->IsKeyReleased = !!physicalKeyStatus.m_bIsKeyReleased;

    return S_OK;
}

