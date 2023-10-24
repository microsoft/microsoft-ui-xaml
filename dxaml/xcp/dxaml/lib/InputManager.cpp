// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputManager.g.h"
#include "InputServices.h"

using namespace DirectUI;

_Check_return_ HRESULT
InputManagerFactory::GetLastInputDeviceTypeImpl(
    _Out_ xaml_input::LastInputDeviceType* pReturnValue)
{
    // MSFT:19551633
    CContentRoot* contentRoot = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot();
    *pReturnValue = static_cast<xaml_input::LastInputDeviceType>(contentRoot->GetInputManager().GetLastInputDeviceType());

    return S_OK;
}

