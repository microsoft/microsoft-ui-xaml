// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Error related types definition.
// They are shared by both Core and Control dlls.

#pragma once

#include "xcperrorresource.h"
#include <xstring_ptr.h>
#include "ErrorType.h"

class CDependencyObject;
struct IError;
class CError;
class CEventArgs;
class CErrorEventArgs;
struct IErrorService;
class CCoreServices;

struct IErrorServiceListener : public IObject
{
    virtual void NotifyErrorAdded(HRESULT hrToOriginate, _In_ IErrorService* pErrorService) = 0;
};

//------------------------------------------------------------------------
//
//  struct IErrorService
//
//  Synopsis:
//      An interface on how to manipulate Error information tracked in the
//      core service.
//
//------------------------------------------------------------------------

struct IErrorService
{
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual _Check_return_ HRESULT AddListener(_In_ IErrorServiceListener* pListener) = 0;
    virtual _Check_return_ HRESULT RemoveListener(_In_ IErrorServiceListener* pListener) = 0;

    // Non Parser error information.

    virtual _Check_return_ HRESULT GetMessageFromErrorCode(
        XUINT32 iErrorID,
        _Out_ xstring_ptr* pstrMessage
        ) = 0;

    virtual _Check_return_ HRESULT  GetLastReportedError(_Outptr_ IError **ppError) = 0;
    virtual _Check_return_ HRESULT  GetFirstError(_Outptr_ IError **ppError) = 0;

    virtual _Check_return_ HRESULT AddError(HRESULT hrToOriginate, _In_ IError *pErrorObject) = 0;

    virtual _Check_return_ HRESULT  CleanupLastError(_In_ IError *pError) = 0;

    virtual void CleanupErrors( ) = 0;

    virtual void CoreResetCleanup( _In_ CCoreServices* pCore ) = 0;

    virtual _Check_return_ HRESULT ReportGenericError (
                HRESULT hrToOriginate,
                ErrorType eType,
                XUINT32 iErrorCode,
                XUINT32 bRecoverable,
                XUINT32 uLineNumber,
                XUINT32 uCharPosition,
                _In_reads_(cParams) const xephemeral_string_ptr* pParams,
                _In_ XUINT32 cParams = 0,
                _In_opt_ CEventArgs *pErrorEventArgs = NULL,
                _In_opt_ CDependencyObject *pSender = NULL
                ) = 0;


    virtual _Check_return_ HRESULT ReportParserError(
                HRESULT hrToOriginate,
                XUINT32 iErrorCode,
                XUINT32 bRecoverable,
                _In_ const xstring_ptr& strXamlFileName,
                _In_ const xstring_ptr& strXmlElement,
                _In_ const xstring_ptr& strXmlAttribute,
                XUINT32 uLineNumber,
                XUINT32 uCharPosition,
                _In_ const xstring_ptr& strMessage,
                _In_reads_(cParams) const xephemeral_string_ptr* pParams,
                _In_ XUINT32 cParams
                ) = 0;

};

struct IError
{
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual void SetErrorEventSender(_In_ CDependencyObject *pSender) = 0;
    virtual _Check_return_ HRESULT GetErrorEventSender(_Outptr_ CDependencyObject **ppSender)= 0;

    virtual _Check_return_ HRESULT SetErrorEventArgs(_In_ CEventArgs *pErrorEventArgs)= 0;

    virtual HRESULT   GetErrorMessage(_Out_ xstring_ptr* pstrErrorMessage)= 0;
    virtual HRESULT   SetErrorMessage(_In_ const xstring_ptr& strErrorMessage)= 0;

    virtual ErrorType GetErrorType()= 0;
    virtual XUINT32   GetErrorCode()= 0;
    virtual HRESULT   GetErrorResult()= 0;
    virtual XUINT32   GetLineNumber()= 0;
    virtual XUINT32   GetCharPosition()= 0;
    virtual XUINT32   GetIsRecoverable() = 0;
    virtual XUINT32   SetIsRecoverable(_In_ XUINT32 bIsRecoverable) = 0;
};

// AG Error codes can be encoded as HRESULTS by setting the high order bits.

HRESULT AgError(XUINT32 agErrorCode);
XUINT32 AgCodeFromHResult(HRESULT hr);

struct HResultToECodePair
{
   HRESULT   m_hrErr;
   XUINT32   m_iErrorCode;
};

struct ECodePair
{
    XUINT32  m_iKey;
    XUINT32  m_iEquivalent;
};