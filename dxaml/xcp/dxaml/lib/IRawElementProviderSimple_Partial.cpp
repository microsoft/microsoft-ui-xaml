// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IRawElementProviderSimple.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the RawElementProviderSimple class.
DirectUI::IRawElementProviderSimple::IRawElementProviderSimple()
    : m_pAutomationPeer(NULL)
{
}

DirectUI::IRawElementProviderSimple::IRawElementProviderSimple(
    xaml_automation_peers::IAutomationPeer* pAutomationPeer)
{
    m_pAutomationPeer = pAutomationPeer;
    AddRefInterface(m_pAutomationPeer);
}

// Destroys an instance of the ButtonBase class.
DirectUI::IRawElementProviderSimple::~IRawElementProviderSimple()
{
    ReleaseInterface(m_pAutomationPeer);
}

_Check_return_ HRESULT DirectUI::IRawElementProviderSimple::GetAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;    

    IFCPTR(ppAutomationPeer);

    *ppAutomationPeer = m_pAutomationPeer;
    AddRefInterface(*ppAutomationPeer);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::IRawElementProviderSimple::SetAutomationPeer(
    _In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer)
{
    ReleaseInterface(m_pAutomationPeer);        
    m_pAutomationPeer = pAutomationPeer;
    AddRefInterface(m_pAutomationPeer);
    RRETURN(S_OK);
}
