// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "CKeyboardAcceleratorInvokedEventArgs.h"

_Check_return_ HRESULT CKeyboardAcceleratorInvokedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateKeyboardAcceleratorInvokedEventArgs(this, ppPeer));
}

