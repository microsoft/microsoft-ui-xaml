// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementDiagnostics.h"
#include <winmeta.h>
#include <TraceLoggingProvider.h>
#include <cstdio>

TRACELOGGING_DECLARE_PROVIDER(g_hTraceProvider);

namespace DirectUI::WindowPlacementPersistence
{
#if WINDOWPLACEMENT_TESTS
    namespace
    {
        thread_local PlacementFailureObserver s_observer{};
    }

    PlacementFailureObserver SetPlacementFailureObserverForTesting(PlacementFailureObserver observer) noexcept
    {
        const auto previous = s_observer;
        s_observer = observer;
        return previous;
    }
#endif

    void ReportPlacementFailure(
        PlacementOperation operation, PlacementFailureCategory category, HRESULT error) noexcept
    {
        TraceLoggingWrite(g_hTraceProvider, "WindowPlacementFailure",
            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
            TraceLoggingUInt8(static_cast<uint8_t>(operation), "Operation"),
            TraceLoggingUInt8(static_cast<uint8_t>(category), "Category"),
            TraceLoggingHResult(error, "Error"));
#if DBG
        wchar_t message[128]{};
        swprintf_s(message, L"WinUI window placement failure: operation=%u category=%u error=0x%08lX\n",
            static_cast<unsigned>(operation), static_cast<unsigned>(category),
            static_cast<unsigned long>(error));
        ::OutputDebugStringW(message);
#endif
#if WINDOWPLACEMENT_TESTS
        if (s_observer) s_observer(operation, category, error);
#endif
    }
}
