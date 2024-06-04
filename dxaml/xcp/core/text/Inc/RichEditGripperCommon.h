// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichEditGripperCommon
{
    enum class GripperType
    {
        Start = 0,
        End,
    };

    enum class PoleMode
    {
        Hidden = 0,
        Selection,
        Pcp,
    };
}