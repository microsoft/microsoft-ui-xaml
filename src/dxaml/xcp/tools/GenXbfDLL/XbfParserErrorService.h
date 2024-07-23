// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class XbfParserErrorService final : public ParserErrorReporter
{
public:
    XbfParserErrorService();
    virtual ~XbfParserErrorService();

    void Initialize(_In_ CCoreServices* pCore);

    XUINT32 GetErrorCode()   { return m_uiErrorCode; }
    XUINT32 GetErrorLine()   { return m_uiLine; }
    XUINT32 GetErrorColumn() { return m_uiColumn; }

protected:
    virtual HRESULT ReportError(
            _In_ XUINT32 iErrorCode,
            _In_ XUINT32 uLine,
            _In_ XUINT32 uColumn,
            _In_ XUINT32 uParamCount,
            _In_ XUINT32 bRecoverable,
            _In_ const xstring_ptr& spMessage);

private:
    CCoreServices* m_pCore{};
    xstring_ptr m_spMessage;
    XUINT32 m_uiErrorCode{};
    XUINT32 m_uiLine{};
    XUINT32 m_uiColumn{};
};

