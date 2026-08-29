// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Math helpers and utilities.

#pragma once

namespace RichTextServices
{
    namespace Internal
    {
        class Math
        {
            //------------------------------------------------------
            //
            //  Public Methods
            //
            //------------------------------------------------------
        public:
            static XINT32 Max(_In_ XINT32 x, _In_ XINT32 y);
            static XINT32 Min(_In_ XINT32 x, _In_ XINT32 y);
            static XUINT32 Max(_In_ XUINT32 x, _In_ XUINT32 y);
            static XUINT32 Min(_In_ XUINT32 x, _In_ XUINT32 y);
            static XFLOAT Max(_In_ XFLOAT x, _In_ XFLOAT y);
            static XFLOAT Min(_In_ XFLOAT x, _In_ XFLOAT y);
            static XINT32 Floor(_In_ XDOUBLE x); 
            static XINT32 Round(_In_ XDOUBLE x); 
        };

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        inline XINT32 Math::Max(_In_ XINT32 x, _In_ XINT32 y)
        { 
            return x > y ? x : y;
        }

        inline XINT32 Math::Min(_In_ XINT32 x, _In_ XINT32 y)
        { 
            return x < y ? x : y;
        }

        inline XUINT32 Math::Max(_In_ XUINT32 x, _In_ XUINT32 y)
        { 
            return x > y ? x : y;
        }

        inline XUINT32 Math::Min(_In_ XUINT32 x, _In_ XUINT32 y)
        { 
            return x < y ? x : y;
        }

        inline XFLOAT Math::Max(_In_ XFLOAT x, _In_ XFLOAT y)
        { 
            return x > y ? x : y;
        }

        inline XFLOAT Math::Min(_In_ XFLOAT x, _In_ XFLOAT y)
        { 
            return x < y ? x : y;
        }

        inline XINT32 Math::Round(_In_ XDOUBLE x)
        {
            return Math::Floor(x+0.5);
        }

        inline XINT32 Math::Floor(_In_ XDOUBLE x)
        {
            XINT32 result;

            const XINT32 nMaxInt = 2147483647;
            const XINT32 nMinInt = (-2147483647 - 1);
            const XFLOAT rMaxInt = (XFLOAT)nMaxInt;
            const XFLOAT rMinInt = (XFLOAT)nMinInt;

            if (x >= rMaxInt)
            {
                result = nMaxInt;
            }
            else if (x <= rMinInt)
            {
                result = nMinInt;
            }
            else
            {
                union {
                    XDOUBLE d;
        #if BIGENDIAN
                    struct {
                        XINT32 msb;
                        XUINT32 lsb;
                    } i;
        #else
                    struct {
                        XUINT32 lsb;
                        XINT32 msb;
                    } i;
        #endif
                } di;

                di.d = (x-.25) + (0x000C000000000000LL);
                result = (di.i.lsb >> 1) | (di.i.msb << 31);
                // TODO: 100953 - fix underlying fp precision issues causing
                // this assert to fail in powerpoint hosted apps.
                //ASSERT(x >= result && x < result + 1);
            }

            return result;
        }
    }
}
