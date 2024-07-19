// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template <typename T, std::size_t expected, std::size_t actual>
void validate_size()
{
    static_assert(expected == actual, "Size of type changed.");
};

template <typename T, std::size_t expected, std::size_t actual>
void validate_alignment()
{
    static_assert(expected == actual, "Alignment requirement of type changed.");
};

#if defined(_X86_)
  #if defined(DBG)
    #define VALIDATE_TYPE_LAYOUT(class_name, x86fre_size, x86chk_size, x64fre_size) \
    template void validate_size<class_name, x86chk_size, sizeof(class_name)>();
  #else
    #define VALIDATE_TYPE_LAYOUT(class_name, x86fre_size, x86chk_size, x64fre_size) \
    template void validate_size<class_name, x86fre_size, sizeof(class_name)>();
  #endif
#elif defined(_AMD64_) && !defined(DBG)
  #define VALIDATE_TYPE_LAYOUT(class_name, x86fre_size, x86chk_size, x64fre_size) \
  template void validate_size<class_name, x64fre_size, sizeof(class_name)>();
#else
  #define VALIDATE_TYPE_LAYOUT(class_name, x86fre_size, x86chk_size, x64fre_size)
#endif