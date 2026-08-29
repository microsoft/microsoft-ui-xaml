// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "wil\resource.h"
#include "macros.h"
#include "microsoft.diagnostics.appanalysis.internal.h"
#include <windowscollections.h>
#include <vector>
#include <algorithm>

// This file is used to help make managing lifetime of some AppAnalysis objects
// a little easier. These function declarations are used for the unique_any_struct
// template and are made to sort of mimick Win32 APIs like CloseHandle/CloseWindow
// (used for unique_handle/unique_hwnd).

// Forwared decleration of COM allocators and deallocators

void CoDeallocSourceInfo(_Inout_ appanalysis::SourceInfo* info);
HRESULT CoAllocSourceInfo(_In_ const appanalysis::SourceInfo& lhs, _Out_ appanalysis::SourceInfo* rhs);

// helper wil functions and definitions for managing our structs
namespace wil {

    typedef unique_struct<appanalysis::SourceInfo, decltype(&::CoDeallocSourceInfo), ::CoDeallocSourceInfo> unique_sourceinfo;

    template <typename struct_t, typename close_fn_t, close_fn_t close_fn, typename dup_fn_t, dup_fn_t dup_fn>
    class shared_struct : public unique_struct<struct_t, close_fn_t, close_fn>
    {
    public:
        shared_struct()
        {
        }

        shared_struct(const shared_struct& other)
        {
            dup_fn(other, this);
        }

        shared_struct& operator=(const shared_struct& other)
        {
            if (this != &other)
            {
                dup_fn(other, this);
            }
            return *this;
        }

        shared_struct& operator=(const struct_t& other)
        {
            if (*this != &other)
            {
                dup_fn(other, this);
            }
            return this;
        }

    private:
        shared_struct(const unique_struct&) = delete;
        shared_struct& operator=(const unique_struct&) = delete;
    };

    typedef shared_struct<appanalysis::SourceInfo, decltype(&::CoDeallocSourceInfo), ::CoDeallocSourceInfo, decltype(&::CoAllocSourceInfo), ::CoAllocSourceInfo> shared_sourceinfo;

}; // namespace wil
