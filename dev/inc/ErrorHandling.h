// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <exception>

#ifndef E_INVALID_OPERATION
#define E_INVALID_OPERATION HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION)
#endif

// CATCH_RETURN macro should be used in all ABI methods like this:
// IFACEMETHODIMP CFoo::Bar(...) try
// {
//     ...
//     CATCH_RETURN
// } 
//

#define CATCH_RETURN \
        return S_OK; \
    } catch (...) { \
        const auto hr = winrt::to_hresult(); \
        __analysis_assume(FAILED(hr)); \
        return hr; 
