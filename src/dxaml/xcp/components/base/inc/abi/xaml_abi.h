// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if defined(MIDL_NS_PREFIX) && !defined(__cplusplus_winrt)
    #define XAML_ABI_PARAMETER(x) ABI::x
    #define XAML_ABI_NAMESPACE_BEGIN namespace ABI {
    #define XAML_ABI_NAMESPACE_END }
#else
    #define XAML_ABI_PARAMETER(x) x
    #define XAML_ABI_NAMESPACE_BEGIN
    #define XAML_ABI_NAMESPACE_END
#endif // defined(MIDL_NS_PREFIX)