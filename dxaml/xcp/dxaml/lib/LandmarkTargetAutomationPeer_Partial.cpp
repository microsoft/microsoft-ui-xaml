// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LandmarkTargetAutomationPeer.g.h"
using namespace DirectUI;

IFACEMETHODIMP LandmarkTargetAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"LandmarkTarget")).CopyTo(pReturnValue));

    return S_OK;
}

IFACEMETHODIMP LandmarkTargetAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;
    return S_OK;
}
