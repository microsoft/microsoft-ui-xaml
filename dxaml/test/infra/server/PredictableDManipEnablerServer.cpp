// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PredictableDManipEnablerServer.h"

using namespace WEX::Common;

PredictableDManipEnablerServer::PredictableDManipEnablerServer()
{
    // These DManip keys need to be set in the 64-bit hive, even when the test is 32-bit. Ask for KEY_WOW64_64KEY
    // explicitly. This flag will be ignored when running on 32-bit OSes.
    DWORD disposition = 0;
    auto status = ::RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\DirectManipulation",
        0,
        nullptr,
        0,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        nullptr,
        &m_hKey,
        &disposition);
    Throw::LastErrorIf(status != ERROR_SUCCESS);
    if (disposition == REG_OPENED_EXISTING_KEY)
    {
        LOG_WARNING(L"Registry key HLM\\Software\\Microsoft\\Windows\\CurrentVersion\\DirectManipulation already existed");
    }

    DWORD data = 1;
    status = ::RegSetValueEx(
        m_hKey,
        L"ManipulationExact",
        0,
        REG_DWORD,
        reinterpret_cast<BYTE*>(&data),
        sizeof(DWORD));
    Throw::LastErrorIf(status != ERROR_SUCCESS);

    data = 1;
    status = ::RegSetValueEx(
        m_hKey,
        L"OverrideHimetric",
        0,
        REG_DWORD,
        reinterpret_cast<BYTE*>(&data),
        sizeof(DWORD));
    Throw::LastErrorIf(status != ERROR_SUCCESS);

    data = 0;
    status = ::RegSetValueEx(
        m_hKey,
        L"Prediction",
        0,
        REG_DWORD,
        reinterpret_cast<BYTE*>(&data),
        sizeof(DWORD));
    Throw::LastErrorIf(status != ERROR_SUCCESS);
}

PredictableDManipEnablerServer::~PredictableDManipEnablerServer()
{
    auto status = ::RegCloseKey(m_hKey);
    Throw::LastErrorIf(status != ERROR_SUCCESS);
    m_hKey = nullptr;

    status = ::RegDeleteKeyEx(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\DirectManipulation",
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        0);
    Throw::LastErrorIf(status != ERROR_SUCCESS);
}
