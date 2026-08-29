// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GCInstrumentationAggregator.h"
#include "XamlTraceLogging.h"
#include <DependencyLocator.h>

namespace Instrumentation{

    std::shared_ptr<CTelemetryAggregatorGC> GetTelemetryAggregator()
    {
        static PROVIDE_DEPENDENCY(CTelemetryAggregatorGC, DependencyLocator::StoragePolicyFlags::None);
        static DependencyLocator::Dependency<Instrumentation::CTelemetryAggregatorGC> s_gcTelemetryAggregator;
        return s_gcTelemetryAggregator.Get();
    }

    CMeasureAggregationGC::CMeasureAggregationGC(ULONG ulMeasureId)
    {
        m_nNextTimeOffset = 0;
        m_nNextTime = 0;
        m_nLogCount = 0;
        m_nSinceLastCounter = 0;
        m_nMaxGCSize = 0;
        m_nLatestGCSize = 0;
        m_nFrequency = 1;

        m_ulMeasureId = ulMeasureId;
    }

    void CMeasureAggregationGC::SetPeriod(LONGLONG nOffset, LONGLONG nStartOffset)
    {
        LARGE_INTEGER endingTime, frequency;
        QueryPerformanceFrequency(&frequency);

        m_nNextTimeOffset = nOffset * frequency.QuadPart;
        m_nFrequency = frequency.QuadPart;


        if (QueryPerformanceCounter(&endingTime))
        {
            m_nNextTime = endingTime.QuadPart + nStartOffset*frequency.QuadPart;
        }
        else
        {
            m_nNextTime = 0;
        }
    }
    
    void CMeasureAggregationGC::AddData(LONGLONG nValue, LONGLONG nSystemTicks)
    {
        m_nSinceLastCounter++;
        m_nLatestGCSize = nValue;

        if (m_nMaxGCSize < nValue)
        {
            m_nMaxGCSize = nValue;
        }

        if (nSystemTicks < m_nNextTime || m_nSinceLastCounter < 1)
        {
            return;
        }

        TRACE(TraceAlways, L"IsNextTelemetryInterval Log");

        LogTelemetry();

        m_nSinceLastCounter = 0;
        m_nNextTime = nSystemTicks + m_nNextTimeOffset;
        m_nMaxGCSize = 0;
        m_nLatestGCSize = 0;

        return;
    }

    void CMeasureAggregationGC::LogTelemetry()
    {
        m_nLogCount++;
        TraceLoggingWrite(g_hTraceProvider,
            "GCDuration",
            TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
            TraceLoggingValue(m_ulMeasureId, "DurationType"),
            TraceLoggingValue(m_nMaxGCSize, "Max"),
            TraceLoggingValue(m_nLatestGCSize, "Latest"),
            TraceLoggingValue(m_nSinceLastCounter, "Count"),
            TraceLoggingValue(m_nLogCount, "LogCount"),
            TraceLoggingValue(m_nFrequency, "TickFrequency")
            );
    }

    CTelemetryAggregatorGC::CTelemetryAggregatorGC() :
        m_treeWalkMeasure(static_cast<ULONG> (GCTelemetryType::TreeWalk)),
        m_UIFinalizerMeasure(static_cast<ULONG> (GCTelemetryType::UIFinalizer))
    {
        m_treeWalkMeasure.SetPeriod(m_nPeriod, m_nStartPeriod);
        m_UIFinalizerMeasure.SetPeriod(m_nPeriod, m_nStartPeriod);
    }

    CTelemetryAggregatorGC::~CTelemetryAggregatorGC()
    {
        m_treeWalkMeasure.LogTelemetry();
        m_UIFinalizerMeasure.LogTelemetry();
    }

    void CTelemetryAggregatorGC::SignalEvent(GCEventSignalType gcSignalType)
    {
        LARGE_INTEGER endingTime;
        if (!QueryPerformanceCounter(&endingTime))
        {
            TRACE(TraceAlways, L"SignalEvent : QueryPerformanceCounter failed");
            m_bInTreeWalkGCLock = false;
            m_bInFinalizerLock = false;
            return;
        }

        switch (gcSignalType)
        {
            case GCEventSignalType::AcquireTreeWalkLock:
            {
                m_bInTreeWalkGCLock = true;
                m_nTreeWalkStart = endingTime.QuadPart;
                break;
            }

            case GCEventSignalType::ReleaseTreeWalkLock:
            {
                if (m_bInTreeWalkGCLock)
                {
                    m_treeWalkMeasure.AddData(endingTime.QuadPart - m_nTreeWalkStart, endingTime.QuadPart);
                }

                m_bInTreeWalkGCLock = false;
                break;
            }

            case GCEventSignalType::AcquireUIFinalizerLock:
            {
                m_bInFinalizerLock = true;
                m_nUIFinalizerStart = endingTime.QuadPart;
                break;
            }

            case GCEventSignalType::ReleaseUIFinalizerLock:
            {
                if (m_bInFinalizerLock)
                {
                    m_UIFinalizerMeasure.AddData(endingTime.QuadPart - m_nUIFinalizerStart,endingTime.QuadPart);
                }

                m_bInFinalizerLock = false;
                break;
            }

            default:
            {
                TRACE(TraceAlways, L"SignalEvent : Unknown signal");
                break;
            }

        }
    }
}