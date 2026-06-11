// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    struct HandleClose
    {
        void operator()(HANDLE handle) const
        {
            if (handle && handle != INVALID_HANDLE_VALUE)
                ::CloseHandle(handle);
        }
    };
}

//! A non-copyable, moveable RAII wrapper around a HANDLE.
typedef std::unique_ptr<void, Private::HandleClose> Handle;

