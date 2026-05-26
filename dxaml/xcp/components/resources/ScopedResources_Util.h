// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class xstring_ptr;
class CDependencyObject;

namespace Resources { namespace ScopedResources
{
    _Check_return_ HRESULT GetRuntimeClassNameFromCoreObject(
        _In_ const CDependencyObject* const coreObject,
        _Out_ xstring_ptr* runtimeClassName);
}}