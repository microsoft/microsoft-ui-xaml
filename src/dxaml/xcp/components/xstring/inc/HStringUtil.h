// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace HStringUtil {

    // Below all apis are copied from windowsstringp.h
    inline bool HasEmbeddedNull(_In_ const HSTRING hString) throw()
    {
        BOOL answer{};
        // Not capturing HRESULT
        ::WindowsStringHasEmbeddedNull(hString, &answer);
        return !!answer;
    }

    inline HRESULT GetLpcwstr(_In_ const HSTRING hString, _Outptr_ LPCWSTR *ppsz) throw()
    {
        if (HasEmbeddedNull(hString))
        {
            *ppsz = nullptr;
            return E_INVALIDARG;
        }
        *ppsz = ::WindowsGetStringRawBuffer(hString, nullptr);
        return S_OK;
    }

    inline LPCWSTR GetRawBuffer(_In_ const HSTRING hString, _Out_opt_ UINT32 *length) throw()
    {
        return WindowsGetStringRawBuffer(hString, length);
    }

}