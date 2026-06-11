// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    ref class SelectionInfoStatics sealed
    {
    private:
        static bool m_bSelectRangeInvoked;
        static bool m_bDeselectRangeInvoked;
        static bool m_bIsSelectedInvoked;
        static bool m_bGetSelectedRangesInvoked;

    public:
        property bool SelectRangeInvoked
        {
            static bool get()
            {
                return m_bSelectRangeInvoked;
            }

            static void set(bool value)
            {
                m_bSelectRangeInvoked = value;
            }
        }

        property bool DeselectRangeInvoked
        {
            static bool get()
            {
                return m_bDeselectRangeInvoked;
            }

            static void set(bool value)
            {
                m_bDeselectRangeInvoked = value;
            }
        }

        property bool IsSelectedInvoked
        {
            static bool get()
            {
                return m_bIsSelectedInvoked;
            }

            static void set(bool value)
            {
                m_bIsSelectedInvoked = value;
            }
        }

        property bool GetSelectedRangesInvoked
        {
            static bool get()
            {
                return m_bGetSelectedRangesInvoked;
            }

            static void set(bool value)
            {
                m_bGetSelectedRangesInvoked = value;
            }
        }

        static void Reset()
        {
            m_bSelectRangeInvoked = false;
            m_bDeselectRangeInvoked = false;
            m_bIsSelectedInvoked = false;
            m_bGetSelectedRangesInvoked = false;
        }
    };

    bool SelectionInfoStatics::m_bSelectRangeInvoked = false;
    bool SelectionInfoStatics::m_bDeselectRangeInvoked = false;
    bool SelectionInfoStatics::m_bIsSelectedInvoked = false;
    bool SelectionInfoStatics::m_bGetSelectedRangesInvoked = false;

} } } } }