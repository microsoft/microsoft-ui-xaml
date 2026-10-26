// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlProfilerVisualValuesUnitTests.h"
#include <XamlLogging.h>
#include <XamlProfilerTracing.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    // EVENT_CONTROL_CODE_DISABLE_PROVIDER / ENABLE_PROVIDER / CAPTURE_STATE from evntrace.h.
    constexpr ULONG c_disableProvider = 0;
    constexpr ULONG c_enableProvider = 1;
    constexpr ULONG c_captureState = 2;

    // WucVisualTreeProfiler::VisualProperty ids (comptree isn't visible to these tests).
    constexpr uint32_t c_offset = 1;
    constexpr uint32_t c_size = 2;
    constexpr uint32_t c_clip = 20;
    constexpr uint32_t c_brush = 21;

    // Calls the provider's enable callback the way ETW does when a session changes. Each test
    // starts and ends with a disable, so it sees only the values it records.
    static void NotifySessionChange(ULONG controlCode)
    {
        XamlProfilerTracing::ProviderEnableCallback(nullptr, controlCode, WINEVENT_LEVEL_VERBOSE, 0, 0, nullptr, nullptr);
    }

    // Records (1, 2, z, 0) as the value of the visual's property.
    static bool Record(uint64_t visualId, uint32_t property, float z)
    {
        return XamlProfilerTracing::RecordVisualProperty(visualId, property, 1.0f, 2.0f, z, 0.0f);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateRepeatedPropertyIsSkipped()
    {
        const uint64_t visualId = 0x1000;
        const uint64_t otherVisualId = 0x1100;
        NotifySessionChange(c_disableProvider);

        VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));
        VERIFY_IS_FALSE(Record(visualId, c_offset, 3.0f));
        VERIFY_IS_TRUE(Record(visualId, c_offset, 4.0f));

        // The sessions' latest value is 4 now, so 3 is a change again.
        VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));

        // Values are kept per visual and property.
        VERIFY_IS_TRUE(Record(otherVisualId, c_offset, 3.0f));
        VERIFY_IS_TRUE(Record(visualId, c_size, 3.0f));

        // Compared bit for bit: -0 and +0 are different values.
        VERIFY_IS_TRUE(Record(visualId, c_offset, 0.0f));
        VERIFY_IS_TRUE(Record(visualId, c_offset, -0.0f));

        // An id with no recorded values is always written.
        const uint32_t unrecordedProperty = XamlProfilerTracing::c_visualScalarPropertyCount;
        VERIFY_IS_TRUE(Record(visualId, unrecordedProperty, 3.0f));
        VERIFY_IS_TRUE(Record(visualId, unrecordedProperty, 3.0f));

        NotifySessionChange(c_disableProvider);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateRepeatedTransformIsSkipped()
    {
        const uint64_t visualId = 0x2000;
        float matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        NotifySessionChange(c_disableProvider);

        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));
        VERIFY_IS_FALSE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));

        matrix[12] = 5.0f;
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));
        VERIFY_IS_FALSE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));

        NotifySessionChange(c_disableProvider);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateOnlyRepeatedClearIsSkipped()
    {
        const uint64_t visualId = 0x3000;
        const uint64_t objectId = 0x3100;
        NotifySessionChange(c_disableProvider);

        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));
        VERIFY_IS_FALSE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));

        // An object is written every time: by now its address could hold another object.
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, objectId));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, objectId));

        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));
        VERIFY_IS_FALSE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_brush, 0));

        // An id with no recorded clear is always written.
        const uint32_t unrecordedProperty = 32;
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, unrecordedProperty, 0));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, unrecordedProperty, 0));

        NotifySessionChange(c_disableProvider);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateVisualCreatedForgetsItsValues()
    {
        const uint64_t visualId = 0x4000;
        const uint64_t otherVisualId = 0x4100;
        const float matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        NotifySessionChange(c_disableProvider);

        VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));
        VERIFY_IS_TRUE(Record(otherVisualId, c_offset, 3.0f));

        // No session is listening, so this writes no event; it only forgets the visual's values.
        XamlProfilerTracing::WucVisualCreated(visualId, 1 /* Visual */);

        VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualTransform(visualId, matrix));
        VERIFY_IS_TRUE(XamlProfilerTracing::RecordVisualObject(visualId, c_clip, 0));
        VERIFY_IS_FALSE(Record(otherVisualId, c_offset, 3.0f));

        NotifySessionChange(c_disableProvider);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateSessionChangeForgetsValues()
    {
        const uint64_t visualId = 0x5000;

        for (const ULONG controlCode : { c_disableProvider, c_enableProvider, c_captureState })
        {
            LOG_OUTPUT(L"Control code %lu", controlCode);
            NotifySessionChange(c_disableProvider);
            VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));
            VERIFY_IS_FALSE(Record(visualId, c_offset, 3.0f));

            NotifySessionChange(controlCode);
            VERIFY_IS_TRUE(Record(visualId, c_offset, 3.0f));
        }

        NotifySessionChange(c_disableProvider);
    }

    void XamlProfilerVisualValuesUnitTests::ValidateCapacityForgetsValues()
    {
        const uint64_t firstVisualId = 0x10000;
        const uint64_t extraVisualId = firstVisualId + XamlProfilerTracing::c_visualValuesCapacity;
        NotifySessionChange(c_disableProvider);

        uint32_t written = 0;
        for (uint64_t visualId = firstVisualId; visualId < extraVisualId; ++visualId)
        {
            written += Record(visualId, c_offset, 3.0f) ? 1 : 0;
        }
        VERIFY_ARE_EQUAL(XamlProfilerTracing::c_visualValuesCapacity, written);

        // Full, but a visual that's already recorded doesn't make room.
        VERIFY_IS_FALSE(Record(firstVisualId, c_offset, 3.0f));

        VERIFY_IS_TRUE(Record(extraVisualId, c_offset, 3.0f));
        VERIFY_IS_TRUE(Record(firstVisualId, c_offset, 3.0f));
        VERIFY_IS_FALSE(Record(extraVisualId, c_offset, 3.0f));

        NotifySessionChange(c_disableProvider);
    }

} } } } }
