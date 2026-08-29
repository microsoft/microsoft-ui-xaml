// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    ref class ItemsRangeInfoStatics sealed
    {
    private:
        static bool m_bRangesChangedInvoked;

    public:
        property bool RangesChangedInvoked
        {
            static bool get()
            {
                return m_bRangesChangedInvoked;
            }

            static void set(bool value)
            {
                m_bRangesChangedInvoked = value;
            }
        }

        static void Reset()
        {
            m_bRangesChangedInvoked = false;
        }
    };

    bool ItemsRangeInfoStatics::m_bRangesChangedInvoked = false;

} } } } }