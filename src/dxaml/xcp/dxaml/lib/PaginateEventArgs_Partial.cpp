// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PaginateEventArgs.g.h"

using namespace DirectUI;

PaginateEventArgs::PaginateEventArgs()
{
    m_pPrintTaskOptionsCore = NULL;
    m_pageNumber = 0;
}
PaginateEventArgs::~PaginateEventArgs()
{
    ReleaseInterface(m_pPrintTaskOptionsCore);
}

_Check_return_ HRESULT PaginateEventArgs::get_PrintTaskOptionsImpl(
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

_Check_return_ HRESULT PaginateEventArgs::put_PrintTaskOptionsImpl(_In_ wgr::Printing::IPrintTaskOptionsCore* value)
{
    ReplaceInterface(m_pPrintTaskOptionsCore, value);
    RRETURN(S_OK);
}

_Check_return_ HRESULT PaginateEventArgs::get_CurrentPreviewPageNumberImpl(_Out_ INT* pValue)
{
    *pValue = m_pageNumber;
    RRETURN(S_OK);
}

_Check_return_ HRESULT PaginateEventArgs::put_CurrentPreviewPageNumberImpl(_In_ INT value)
{
    m_pageNumber = value;
    RRETURN(S_OK);
}

