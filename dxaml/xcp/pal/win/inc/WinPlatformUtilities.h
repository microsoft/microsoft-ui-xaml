// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      Singleton class to implement platform utilities

class CWinPlatformUtilities
    final : public CommonPlatformUtilities
{
private:
    CWinPlatformUtilities() {}
public:
    static CWinPlatformUtilities* getInstance()
    {
        static CWinPlatformUtilities onlyInstance;
        return &onlyInstance;
    }

    XUINT32 Xswprintf_s(
        _Out_writes_z_(cchBuf) WCHAR *pBuf,
        XUINT32 cchBuf,
        _In_z_ const WCHAR* pFormat,
        ...
        ) override;

    XUINT32 vXswprintf_s(
        _Out_writes_z_(cchBuf) WCHAR *pBuf,
        XUINT32 cchBuf,
        _In_z_ const WCHAR* pFormat,
        _In_ va_list args
        ) override;

    bool IsUILanguageRTL() override;
};
