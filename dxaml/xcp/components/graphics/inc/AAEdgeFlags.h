// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum AAEdge
{
     // No edge adjustments (aliased)
    AAEdge_None = 0x00,

    // Standard AA edge adjustments
    AAEdge_Bottom = 0x03,
    AAEdge_Right = 0x0c,
    AAEdge_Top = 0x30,
    AAEdge_Left = 0xc0,

    // Determines if the edge will be adjusted outward by 0.5
    AAExterior_Bottom = 0x02,
    AAExterior_Right = 0x08,
    AAExterior_Top = 0x20,
    AAExterior_Left = 0x80,
    // Note: this extension could break out of a clip. For example, if the clip cuts off the top-left piece of a ninegrid exactly,
    // then this could cause the top piece to render 1px into that clip.

    // Determines if the edge will be adjusted inward by 0.5
    AAInterior_Bottom = 0x01,
    AAInterior_Right = 0x04,
    AAInterior_Top = 0x10,
    AAInterior_Left = 0x40,

    AAEdge_All = 0xff,
};
