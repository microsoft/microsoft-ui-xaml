// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

ParserErrorReporter::ParserErrorReporter()
    : m_bErrorRecorded(FALSE)
{
}

ParserErrorReporter::~ParserErrorReporter()
{

}

void ParserErrorReporter::Initialize()
{
    // Reset this if someone sets the core again.
    m_bErrorRecorded = FALSE;
}

bool ParserErrorReporter::IsErrorRecorded()
{
    return m_bErrorRecorded;
}

void ParserErrorReporter::SetIsErrorRecorded(bool bIsErrorRecorded)
{
    m_bErrorRecorded = bIsErrorRecorded;
}


HRESULT ParserErrorReporter::SetError(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ XUINT32 bRecoverable)
{
    xstring_ptr pstrIgnored;
    IFC_RETURN(ReportError(iErrorCode, uLine, uColumn, 0, bRecoverable, pstrIgnored));

    return S_OK;
}


HRESULT ParserErrorReporter::SetError(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ const xstring_ptr& pstrParam1)
{
    xstring_ptr pstrIgnored;
    ASSERT(m_ErrorParams[0].IsNull());
    IFC_RETURN(LoadAdjustedStringBuffer(pstrParam1, 0));
    IFC_RETURN(ReportError(iErrorCode, uLine, uColumn, 1, /* bRecoverable */ FALSE, pstrIgnored));
    
    return S_OK;

}


HRESULT ParserErrorReporter::SetError(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ const xstring_ptr& pstrParam1,
    _In_ const xstring_ptr& pstrParam2)
{
    xstring_ptr pstrIgnored;
    ASSERT(m_ErrorParams[0].IsNull());
    ASSERT(m_ErrorParams[1].IsNull());
    IFC_RETURN(LoadAdjustedStringBuffer(pstrParam1, 0));
    IFC_RETURN(LoadAdjustedStringBuffer(pstrParam2, 1));
    IFC_RETURN(ReportError(iErrorCode, uLine, uColumn, 2, /* bRecoverable */ FALSE, pstrIgnored));

    return S_OK;
}

HRESULT ParserErrorReporter::SetErrorWithMessage(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ XUINT32 bRecoverable,
    _In_ const xstring_ptr& spMessage)
{
    IFC_RETURN(ReportError(iErrorCode, uLine, uColumn, 0, bRecoverable, spMessage));

    return S_OK;
}

HRESULT ParserErrorReporter::LoadAdjustedStringBuffer(_In_ const xstring_ptr& inssErrorString, XUINT32 uParamIndex)
{
    // This method will return the naked pointers for handing to the error service.
    //
    // If the string begins with a '.', '!', or '?' then skip it.

    if (!inssErrorString.IsNullOrEmpty())
    {
        XUINT32 cBuffer;
        const WCHAR* pBuffer = inssErrorString.GetBufferAndCount(&cBuffer);
    
        ASSERT(cBuffer > 0); // per the emptiness check above

        // String is at least 1 char long.
        WCHAR c = pBuffer[0];

        if ((c == L'.') || (c == L'?') || (c == L'!'))
        {
            pBuffer++;
            cBuffer--;
        }

        IFC_RETURN(xstring_ptr::CloneBuffer(pBuffer, cBuffer, &(m_ErrorParams[uParamIndex])));
    }
    else
    {
        m_ErrorParams[uParamIndex] = xstring_ptr::EmptyString();
    }
    
    return S_OK;
}







// This is the core implementation.
ParserErrorService::ParserErrorService()
    : m_pCore(NULL)
{
    XCP_WEAK(&m_pCore);
}

ParserErrorService::~ParserErrorService()
{

}
void ParserErrorService::Initialize(_In_ IParserCoreServices* pCore)
{
    m_pCore = pCore;
    ParserErrorReporter::Initialize();
}

HRESULT ParserErrorService::ReportError(
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 uLine,
    _In_ XUINT32 uColumn,
    _In_ XUINT32 uParamCount,
    _In_ XUINT32 bRecoverable,
    _In_ const xstring_ptr& strMessage)
{
    HRESULT hr = S_OK;
    IErrorService *pErrorService = NULL;

    if(IsErrorRecorded())
    {
        goto Cleanup;
    }

    IFCEXPECT_ASSERT(uParamCount <= MAX_ERROR_PARAMS);
    IFC(m_pCore->GetParserErrorService(&pErrorService));

    if(pErrorService)
    {
        xephemeral_string_ptr localErrorParams[MAX_ERROR_PARAMS];

        for (XUINT32 i = 0; i < uParamCount; ++i)
        {
            m_ErrorParams[i].Demote(&localErrorParams[i]);
        }

        // Parser errors will always OriginateError as E_XAMLPARSEFAILED.  Callers like XamlReader.Load will need
        // to make sure to return this hr if they want this this error included in any thrown exception.
        #define E_XAMLPARSEFAILED 10L
        HRESULT hrToOriginate = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);

        IFC(pErrorService->ReportParserError(hrToOriginate, iErrorCode, bRecoverable, xstring_ptr::NullString(), xstring_ptr::NullString(), xstring_ptr::NullString(), uLine, uColumn, strMessage, localErrorParams, uParamCount));

        //
        // Indicate the Error Information is recorded.
        // This will prevent an Unknown_Parser error from reported again.
        //
        SetIsErrorRecorded(TRUE);
    }



Cleanup:
    for (XUINT32 i = 0; i < MAX_ERROR_PARAMS; i++)
    {
        m_ErrorParams[i].Reset();
    }
    RRETURN(hr);
}
