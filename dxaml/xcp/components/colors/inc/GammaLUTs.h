// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//-----------------------------------------------------------------------------
//
//  Abstract:
//      Look-up tables for gamma conversion (converting a color channel
//      from sRGB to scRGB, or vice versa).
//
//-----------------------------------------------------------------------------

const UINT32 GammaLUT_scRGB_to_sRGB_size = 3355;
const FLOAT GammaLUT_scRGB_to_sRGB_scale = 3354.0f;

extern const UINT8 GammaLUT_scRGB_to_sRGB [GammaLUT_scRGB_to_sRGB_size];

// Convert a color channel from scRGB to sRGB byte (0 to 255)
UINT8 Convert_scRGB_Channel_To_sRGB_Byte(FLOAT rColorComponent);

// Return smallest argument for Convert_scRGB_Channel_To_sRGB_Byte that causes nonzero result
FLOAT Get_Smallest_scRGB_Significant_for_sRGB();

extern const FLOAT GammaLUT_sRGB_to_scRGB [256];
