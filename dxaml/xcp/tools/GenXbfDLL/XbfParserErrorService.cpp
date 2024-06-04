// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// This is the core implementation.
XbfParserErrorService::XbfParserErrorService()
    : m_pCore(NULL)
{
    XCP_WEAK(&m_pCore);
}

XbfParserErrorService::~XbfParserErrorService()
{

}
void XbfParserErrorService::Initialize(_In_ CCoreServices* pCore)
{
    m_pCore = pCore;
    ParserErrorReporter::Initialize();
}

HRESULT XbfParserErrorService::ReportError(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ XUINT32 uParamCount,
    _In_ XUINT32 bRecoverable,
    _In_ const xstring_ptr& spMessage)
{
    HRESULT hr = S_OK;
    xstring_ptr strMessage;

    if(IsErrorRecorded())
    {
        goto Cleanup;
    }

    IFCEXPECT_ASSERT(uParamCount <= MAX_ERROR_PARAMS);

    m_spMessage = spMessage;
    m_uiErrorCode = iErrorCode;
    m_uiLine = uLine;
    m_uiColumn = uColumn;

    //IFC(pErrorService->ReportParserError(iErrorCode, bRecoverable, NULL, NULL, NULL, uLine, uColumn, strMessage, m_ErrorParams, uParamCount));

    //
    // Indicate the Error Information is recorded.
    // This will prevent an Unknown_Parser error from reported again.
    //
    SetIsErrorRecorded(TRUE);

Cleanup:
    for (XUINT32 i = 0; i < MAX_ERROR_PARAMS; i++)
    {
        m_ErrorParams[i].Reset();
    }

    RRETURN(hr);
}
