// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "NamedContainerAutomationPeer.g.h"
using namespace DirectUI;

IFACEMETHODIMP NamedContainerAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    IFCPTR_RETURN(returnValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"NamedContainerAutomationPeer")).CopyTo(returnValue));
    return S_OK;
}

IFACEMETHODIMP NamedContainerAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = xaml_automation_peers::AutomationControlType_Group;
    return S_OK;
}
