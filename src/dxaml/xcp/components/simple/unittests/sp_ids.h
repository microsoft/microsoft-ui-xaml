// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace SimpleProperty
{
    enum class PID
    {
        valueInSparse,
        valueInField,
        valueInGroupSparse,
        smartInSparse,
        smartInField,
        smartInGroupSparse,
        bigImmutableInSparse,
        bigImmutableInField,
        bigImmutableInGroupSparse,
        bigMutableInSparse,
        bigMutableInField,
        bigMutableInGroupSparse,
    };

    enum class TID
    {
        Integer,
        SharedBigStructOfInt,
        BigStructOfInt,
        BigStructOfChar
    };
}