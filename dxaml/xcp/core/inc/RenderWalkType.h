// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum RenderWalkType
{
    RWT_None,

    RWT_WinRTComposition,       // WinRT SpriteVisual-oriented code path
    RWT_NonePreserveDComp
};