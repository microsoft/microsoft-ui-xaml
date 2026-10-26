// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlProfilerTypeTableUnitTests.h"
#include <XamlLogging.h>
#include <XamlProfilerTracing.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    // EVENT_CONTROL_CODE_DISABLE_PROVIDER / ENABLE_PROVIDER / CAPTURE_STATE from evntrace.h.
    constexpr ULONG c_disableProvider = 0;
    constexpr ULONG c_enableProvider = 1;
    constexpr ULONG c_captureState = 2;

    // Rows RecordRow has written, per type index. The type table never forgets a type, and a
    // re-send includes the types of tests that ran earlier, so each test uses its own type indices
    // and checks only those.
    static uint32_t s_rowCounts[XamlProfilerTracing::c_typeTableCapacity];
    static uint32_t s_totalRows;

    // When set, RecordRow starts a session while it writes this type's row.
    static uint32_t s_startSessionDuringRowOf = UINT32_MAX;

    // Calls the provider's enable callback the way ETW does when a session changes.
    static void NotifySessionChange(ULONG controlCode)
    {
        XamlProfilerTracing::ProviderEnableCallback(nullptr, controlCode, WINEVENT_LEVEL_VERBOSE, 0, 0, nullptr, nullptr);
    }

    // The TypeRowWriter under test: counts the row instead of writing it.
    static void RecordRow(uint32_t typeIndex) noexcept
    {
        ++s_rowCounts[typeIndex];
        ++s_totalRows;
        if (typeIndex == s_startSessionDuringRowOf)
        {
            s_startSessionDuringRowOf = UINT32_MAX;
            NotifySessionChange(c_enableProvider);
        }
    }

    static void ClearRows()
    {
        for (uint32_t& count : s_rowCounts)
        {
            count = 0;
        }
        s_totalRows = 0;
    }

    void XamlProfilerTypeTableUnitTests::ValidateTypeIsDescribedOnce()
    {
        const uint32_t typeIndex = 101;
        ClearRows();

        XamlProfilerTracing::DescribeType(typeIndex, &RecordRow);
        XamlProfilerTracing::DescribeType(typeIndex, &RecordRow);

        VERIFY_ARE_EQUAL(1u, s_rowCounts[typeIndex]);
    }

    void XamlProfilerTypeTableUnitTests::ValidateSessionStartResendsRows()
    {
        const uint32_t typeIndex = 102;
        XamlProfilerTracing::DescribeType(typeIndex, &RecordRow);

        for (const ULONG controlCode : { c_enableProvider, c_captureState })
        {
            LOG_OUTPUT(L"Control code %lu", controlCode);
            ClearRows();
            NotifySessionChange(controlCode);
            VERIFY_ARE_EQUAL(1u, s_rowCounts[typeIndex]);
        }

        // The sessions that remain already have their rows.
        ClearRows();
        NotifySessionChange(c_disableProvider);
        VERIFY_ARE_EQUAL(0u, s_totalRows);

        // A re-send doesn't make the type need its row again.
        XamlProfilerTracing::DescribeType(typeIndex, &RecordRow);
        VERIFY_ARE_EQUAL(0u, s_totalRows);
    }

    void XamlProfilerTypeTableUnitTests::ValidateSessionStartDuringRowWrite()
    {
        const uint32_t typeIndex = 103;
        ClearRows();

        // The session starts after the row is written but before the type is marked, so the
        // callback's re-send can't include the type. DescribeType must notice and write it again.
        s_startSessionDuringRowOf = typeIndex;
        XamlProfilerTracing::DescribeType(typeIndex, &RecordRow);

        VERIFY_ARE_EQUAL(2u, s_rowCounts[typeIndex]);

        // A session that starts after the type was marked gets it from the re-send.
        ClearRows();
        NotifySessionChange(c_enableProvider);
        VERIFY_ARE_EQUAL(1u, s_rowCounts[typeIndex]);
    }

} } } } }