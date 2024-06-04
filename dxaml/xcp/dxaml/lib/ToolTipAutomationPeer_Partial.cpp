// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToolTipAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the ToolTipAutomationPeer class.
ToolTipAutomationPeer::ToolTipAutomationPeer()
{
}

// Deconstructor
ToolTipAutomationPeer::~ToolTipAutomationPeer()
{
}

IFACEMETHODIMP ToolTipAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ToolTip")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToolTipAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_ToolTip;
    RRETURN(S_OK);
}
