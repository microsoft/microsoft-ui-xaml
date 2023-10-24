// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "XamlTraceLogging.h"
#include <DependencyLocator.h>
#include <sysinfoapi.h>


namespace Instrumentation {

    enum class GCTelemetryType
    {
        TreeWalk,
        UIFinalizer,
    };

    enum class GCEventSignalType
    {
        AcquireTreeWalkLock,
        ReleaseTreeWalkLock,
        AcquireUIFinalizerLock,
        ReleaseUIFinalizerLock
    };

    class CMeasureAggregationGC
    {

    private :

        LONGLONG m_nNextTimeOffset;
        LONGLONG m_nNextTime;
        LONGLONG m_nLogCount;
        LONGLONG m_nSinceLastCounter;
        LONGLONG m_nMaxGCSize;
        LONGLONG m_nLatestGCSize;
        ULONG m_ulMeasureId;
        LONGLONG m_nFrequency;

    public:

        CMeasureAggregationGC(ULONG ulMeasureId);
        void SetPeriod(LONGLONG nOffset, LONGLONG nStartOffset);
        void AddData(LONGLONG nValue, LONGLONG nSystemTicks);
        void LogTelemetry();
    };

    // Telemetry aggregation class for Garbage Collector events.
    // uuid below is for dependency locator to help create unique thread specific instances.
    class __declspec(uuid("9f97ebce-4edc-41dc-8281-1140b43023cc")) CTelemetryAggregatorGC
    {

    private:

        CMeasureAggregationGC m_treeWalkMeasure;
        CMeasureAggregationGC m_UIFinalizerMeasure;

        LONGLONG m_nTreeWalkStart;
        LONGLONG m_nUIFinalizerStart;

        bool m_bInTreeWalkGCLock;
        bool m_bInFinalizerLock;

        LONGLONG m_nPeriod = 300; //initial period is equal to 5 mins ( 5*60 sec)
        LONGLONG m_nStartPeriod = 60; // at startup log at 1 min ( 60 sec)

    public:

        CTelemetryAggregatorGC();
        ~CTelemetryAggregatorGC();
        void SignalEvent(GCEventSignalType gcSignalType);
    };

    std::shared_ptr<CTelemetryAggregatorGC> GetTelemetryAggregator();
}
