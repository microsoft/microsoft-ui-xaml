// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdarg>
#include <sal.h>
#include <strsafe.h>
#include <windows.h>

// TVDiag — logging helper for TableView code.
//
// LogRetailF is retail-visible. Use ONLY where the control silently swallows something an app
// author would need to diagnose, and where the alternative is behaviour that is indistinguishable
// at runtime from "nothing happened": a consumer event handler that threw, or an edit cancel that
// could not restore the pre-edit value. These are error paths, so the cost is not on any hot path.
//
// PII rule: the %s formatter MUST wrap only HRESULT helpers or exception strings — NEVER cell
// values, column headers, or any user data.

namespace TVDiag
{
    namespace details
    {
        inline void EmitV(_In_z_ _Printf_format_string_ wchar_t const* format, va_list args)
        {
            wchar_t buffer[1024]{};
            StringCchVPrintfW(buffer, ARRAYSIZE(buffer), format, args);
            OutputDebugStringW(buffer);
            OutputDebugStringW(L"\n");
        }
    }

    // Intentionally NOT DBG-gated. See the header comment for when this is appropriate.
    inline void LogRetailF(_In_z_ _Printf_format_string_ wchar_t const* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
        details::EmitV(format, args);
        va_end(args);
    }
}
