// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CxxMock.h>
#include <string>

namespace CxxMock
{

inline std::wstring ConvertToWString(const char* _string)
{
    std::string str(_string);
    std::wstring result(str.size(),L'\0');
    for(size_t i=0;i<str.size();i++)
    {
        result[i] = static_cast<wchar_t>(str[i]);
    }
    return result;
}

}

namespace WEX { namespace TestExecution { namespace Private { namespace MacroVerifyEx {

_Post_equal_to_(false)
static bool UnexpectedMockedCall(const wchar_t* pszOperation, const ErrorInfo& errorInfo, const wchar_t* pszMessage = nullptr)
{
    static const wchar_t c_szFormat[] = L"Unexpected call in mock %s : %s";
    WEX::Common::NoThrowString message;
    message.Format(c_szFormat, pszOperation, pszMessage);
    return WEX::TestExecution::Verify::Fail(message.ToCStrWithFallbackTo(c_szFormat), errorInfo);
}

} } } }

#define VERIFY_CXXMOCK_ACTION(__mock, ...)                                                                         \
{                                                                                                                \
    bool __exceptionHit = false;                                                                                 \
    try                                                                                                          \
    {                                                                                                            \
        __mock;                                                                                 \
    }                                                                                                            \
    catch(CxxMock::CMockObjectError& ex)                                                                         \
    {                                                                                                            \
        std::wstring fileInfo(__WFILE__);                                                                        \
        if (ex.GetFileName())                                                                                    \
        {                                                                                                        \
            fileInfo = CxxMock::ConvertToWString(ex.GetFileName()).c_str();                                      \
        }                                                                                                        \
        WEX::TestExecution::ErrorInfo errorInfo(                                                                 \
            fileInfo.c_str(),                                                                                    \
            __WFUNCTION__, ex.GetLineNumber());                                                                  \
        WEX::TestExecution::Private::MacroVerifyEx::UnexpectedMockedCall(                                        \
            L#__mock,                                                                                            \
            errorInfo,                                                                                           \
            CxxMock::ConvertToWString(ex.GetErrorText()).c_str(),                                                \
            __VA_ARGS__);                                                                                        \
        __exceptionHit = true;                                                                                   \
    }                                                                                                            \
                                                                                                                 \
    if (!__exceptionHit)                                                                                         \
    {                                                                                                            \
        WEX::TestExecution::Private::MacroVerify::UnexpectedExceptionNotThrown(L#__mock, __VA_ARGS__);           \
    }                                                                                                            \
}

#define VERIFY_EXPECTATIONS(__mock, ...) VERIFY_CXXMOCK_ACTION(CxxMock::Verify(__mock),  __VA_ARGS__);
#define VERIFY_SUCCEEDED_WITHMOCKS(__mock, ...) VERIFY_CXXMOCK_ACTION(VERIFY_SUCCEEDED(__mock),  __VA_ARGS__);

