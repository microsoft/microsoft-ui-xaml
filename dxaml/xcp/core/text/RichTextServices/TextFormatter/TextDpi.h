// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helpers to handle text dpi conversions and limitations.

#pragma once

#include "TxMath.h"

namespace RichTextServices
{
    namespace Internal
    {
        // Measuring unit for PTLS is INT, but logical measuring for the rest of 
        // the system is is floating point. To avoid truncation text is using higher DPI.
        // Logical measuring dpi    = 96
        // PTLS measuring dpi       = 28800
        #define TEXTDPI_SCALE 300.0f

        // PTLS measuring dpi
        #define TEXTDPI_DPI 28800

        //---------------------------------------------------------------------------
        //
        //  TextDpi
        //
        //  Helpers to handle text dpi conversions and limitations.
        //
        //---------------------------------------------------------------------------
        struct TextDpi
        {
            // Converts from text measurement unit to logical measurement unit.
            static XFLOAT FromTextDpi(_In_ XINT32 value);

            // Converts from logical measurement unit to text measurement unit.
            static XINT32 ToTextDpi(_In_ XFLOAT value);

            // Returns PTLS measuring dpi.
            static XINT32 Dpi();
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      FromTextDpi
        //
        //  Synopsis:
        //      Converts from text measurement unit to logical measurement unit.
        //
        //---------------------------------------------------------------------------
        inline XFLOAT TextDpi::FromTextDpi(
            _In_ XINT32 value
            )
        {
            return static_cast<XFLOAT>(value) / TEXTDPI_SCALE;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ToTextDpi
        //
        //  Synopsis:
        //      Converts from logical measurement unit to text measurement unit.
        //
        //---------------------------------------------------------------------------
        inline XINT32 TextDpi::ToTextDpi(
            _In_ XFLOAT value
            )
        {
            XINT32 intValue = static_cast<XINT32>(Math::Round(value * TEXTDPI_SCALE));
            if (intValue > tsduRestriction)
            {
                intValue = tsduRestriction;
            }
            else if (intValue < -tsduRestriction)
            {
                intValue = -tsduRestriction;
            }
            return intValue;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      Dpi
        //
        //  Synopsis:
        //      Returns PTLS measuring dpi.
        //
        //---------------------------------------------------------------------------
        inline XINT32 TextDpi::Dpi()
        {
            return TEXTDPI_DPI;
        }
    }
}
