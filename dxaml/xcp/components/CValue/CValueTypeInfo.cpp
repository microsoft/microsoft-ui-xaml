// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValueTypeInfo.h>
#include <xstring_ptr.h>

namespace CValueDetails
{
    // Empty values needing storage
    decltype(ValueTypeInfo<valueEnum>::Empty) ValueTypeInfo<valueEnum>::Empty = { 0, KnownTypeIndex::UnknownType };
    decltype(ValueTypeInfo<valueString>::Empty) ValueTypeInfo<valueString>::Empty = xstring_ptr::NullString();
    decltype(ValueTypeInfo<valueDateTime>::Empty) ValueTypeInfo<valueDateTime>::Empty = {};
    decltype(ValueTypeInfo<valueTimeSpan>::Empty) ValueTypeInfo<valueTimeSpan>::Empty = {};
    decltype(ValueTypeInfo<valueTextRange>::Empty) ValueTypeInfo<valueTextRange>::Empty = {};
    decltype(ValueTypeInfo<valueEnum8>::Empty) ValueTypeInfo<valueEnum8>::Empty = { 0, KnownTypeIndex::UnknownType };
}