// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "minxcptypes.h"
#include "xstring_ptr.h"

#pragma once
class TypeNameHelper
{
    public:
        static _Check_return_ HRESULT ParseAssemblyQualifiedTypeName(
            _In_ const xstring_ptr& strAssemblyQualifiedTypeName,
            _Out_ xstring_ptr* pstrTypeName,
            _Out_ xstring_ptr* pstrAssemblyName);

        static _Check_return_ HRESULT GetNamespace(
            _In_z_ const WCHAR* wszTypeName, 
            _Outptr_result_maybenull_z_ WCHAR** pwszNamespace);

        static _Check_return_ HRESULT IsFrameworkNamespace(
            _In_z_ const WCHAR* wszNamespace, 
            _Out_ bool* fFrameworkNamespace);
};

