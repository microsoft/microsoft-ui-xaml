// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AddPagesEventArgs.g.h"

using namespace DirectUI;

AddPagesEventArgs::AddPagesEventArgs()
{
    m_pPrintTaskOptionsCore = NULL;
}
AddPagesEventArgs::~AddPagesEventArgs()
{
    ReleaseInterface(m_pPrintTaskOptionsCore);
}

_Check_return_ HRESULT AddPagesEventArgs::get_PrintTaskOptionsImpl(
    _Outptr_ wgr::Printing::IPrintTaskOptionsCore** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = NULL;

    AddRefInterface(m_pPrintTaskOptionsCore);
    *pValue = m_pPrintTaskOptionsCore;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AddPagesEventArgs::put_PrintTaskOptionsImpl(_In_ wgr::Printing::IPrintTaskOptionsCore* value)
{
    ReplaceInterface(m_pPrintTaskOptionsCore, value);
    RRETURN(S_OK);
}


