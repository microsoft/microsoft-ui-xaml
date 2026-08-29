// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class IParserCoreServices;

class ParserErrorReporter
{
public:
    ParserErrorReporter();
    virtual ~ParserErrorReporter();

    void Initialize();
    bool IsErrorRecorded();
    void SetIsErrorRecorded(bool bIsErrorRecorded);

    HRESULT SetError(
        _In_ XUINT32 iErrorCode,
        _In_ XUINT32 uLine,
        _In_ XUINT32 uColumn,
        _In_ XUINT32 bRecoverable = FALSE);

    HRESULT SetError(
        _In_ XUINT32 iErrorCode,
        _In_ XUINT32 uLine,
        _In_ XUINT32 uColumn,
        _In_ const xstring_ptr& pstrParam1);

    HRESULT SetError(
        _In_ XUINT32 iErrorCode,
        _In_ XUINT32 uLine,
        _In_ XUINT32 uColumn,
        _In_ const xstring_ptr& pstrParam1,
        _In_ const xstring_ptr& pstrParam2);

    HRESULT SetErrorWithMessage(
        _In_ XUINT32 iErrorCode,
        _In_ XUINT32 uLine,
        _In_ XUINT32 uColumn,
        _In_ XUINT32 bRecoverable,
        _In_ const xstring_ptr& spMessage);

protected:
    virtual HRESULT ReportError(
            _In_ XUINT32 iErrorCode,
            _In_ XUINT32 uLine,
            _In_ XUINT32 uColumn,
            _In_ XUINT32 uParamCount,
            _In_ XUINT32 bRecoverable,
            _In_ const xstring_ptr& spMessage) = 0;

private:
    HRESULT LoadAdjustedStringBuffer(_In_ const xstring_ptr& inssErrorString, XUINT32 uParamIndex);

protected:
    static const XUINT32 MAX_ERROR_PARAMS = 2;
    bool m_bErrorRecorded;
    xstring_ptr m_ErrorParams[MAX_ERROR_PARAMS];


};


class ParserErrorService final : public ParserErrorReporter
{
public:
    ParserErrorService();
    ~ParserErrorService() override;

    void Initialize(_In_ IParserCoreServices* pCore);

protected:
    HRESULT ReportError(
            _In_ XUINT32 iErrorCode,
            _In_ XUINT32 uLine,
            _In_ XUINT32 uColumn,
            _In_ XUINT32 uParamCount,
            _In_ XUINT32 bRecoverable,
            _In_ const xstring_ptr& spMessage) override;


private:
    IParserCoreServices* m_pCore;
};
