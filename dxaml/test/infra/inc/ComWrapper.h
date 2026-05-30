// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  This allows us to translate C++ exceptions into HRESULTs at the API
//  boundary level. It was adopted from a header file used internally in TAEF.

#pragma once

#include <WexAssert.h>

#ifndef COM_GROUP_ISBVT
#define COM_GROUP_ISBVT false
#endif

#define COM_START_GROUP(msg) \
    HRESULT __hr__ = S_OK;                                                                                    \
    const wchar_t __msg__[] = msg;                                                                            \
    const bool __isBVT__ = COM_GROUP_ISBVT;                                                                   \
    try                                                                                                       \
    {                                                                                                         \
        if(__msg__[0] && __isBVT__) WEX::Logging::Log::StartGroup(__msg__);                                   \


#define COM_START COM_START_GROUP(L"")

#define COM_END \
        /* no exceptions encountered */                                                                       \
        if (__msg__[0] && __isBVT__) WEX::Logging::Log::EndGroup(__msg__);                                    \
        return S_OK;                                                                                          \
    }                                                                                                         \
    catch (const WEX::Common::Exception& __e__)                                                               \
    {                                                                                                         \
        /* translate exception to HRESULT */                                                                  \
        if (__isBVT__)                                                                                        \
            LOG_ERROR_SC(L"COM_END %s:%s",__msg__,__e__.Message());                                           \
        else                                                                                                  \
            LOG_ERROR(L"COM_END %s:%s",__msg__,__e__.Message());                                              \
        __hr__ = __e__.ErrorCode();                                                                           \
    }                                                                                                         \
    catch (const std::exception& __e__)                                                                       \
    {                                                                                                         \
        const auto errorString = WEX::Common::NoThrowString()                                                 \
            .Format(L"COM_END %s:Unexpected [%S] std::exception caught", __msg__, __e__.what());              \
        if (__isBVT__)                                                                                        \
            LOG_ERROR_SC(errorString);                                                                        \
        else                                                                                                  \
            LOG_ERROR(errorString);                                                                           \
        __hr__ = E_UNEXPECTED;                                                                                \
    }                                                                                                         \
                                                                                                              \
    /* check for unexpected HRESULT values */                                                                 \
    WEX_ASSERT(FAILED(__hr__), L"Success HRESULT thrown? That should never happen!");                         \
    if (__msg__[0] && __isBVT__) WEX::Logging::Log::EndGroup(__msg__);                                        \
    return __hr__;                                                                                            \

