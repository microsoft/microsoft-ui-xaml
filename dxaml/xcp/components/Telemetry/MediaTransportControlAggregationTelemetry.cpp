// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaTransportControlAggregationTelemetry.h"
#include "XamlTraceLogging.h"

CAggMediaControlEvents::CAggMediaControlEvents()
    : m_actions()
    , m_MEInstanceGuid(GUID_NULL)
    , m_bIsControlPanelVisible(false)
    , m_NumControlPanelTransistions(0)
    , m_CurrentCCTrack(-1)
    , m_NumCCTurnedoff(0)
    , m_NumCCTrackChanged(0)
    , m_ulStart(GetTickCount64())
    , m_ulMS(10000)
    , m_bNewData(false)
{
}

CAggMediaControlEvents::~CAggMediaControlEvents()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Telemetry Aggregation LogData functions send the telemetry once 
//        policy conditions are met
//
//------------------------------------------------------------------------
void 
CAggMediaControlEvents::LogTelemetry()
{
    // Trace telemetry for identifying MediaTransportControl Creation
    TraceLoggingWrite(
        g_hTraceProvider,
        "MediaTransportControlsStatistics",
        TraceLoggingGuid(m_MEInstanceGuid, "ElementInstanceId"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::MuteClick].lastErrorcode, "HRESULTMuteClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::VolumeClick].lastErrorcode, "HRESULTVolumeClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::PlayPauseClick].lastErrorcode, "HRESULTPlayPauseClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::FullWindowClick].lastErrorcode, "HRESULTFullWindowClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::ZoomClick].lastErrorcode, "HRESULTZoomClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::SizeChanged].lastErrorcode, "HRESULTSizeChanged"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::PositionSliderPressed].lastErrorcode, "HRESULTPositionSliderPressed"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::ScrubbingMode].lastErrorcode, "HRESULTScrubbingMode"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::CastButtonClick].lastErrorcode, "HRESULTCastingClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::CCButtonClick].lastErrorcode, "HRESULTClosedCaptionsClick"),
        TraceLoggingHResult(m_actions[(int)MTCTelemetryType::CCTrackClick].lastErrorcode, "HRESULTClosedCaptionsTrackSelect"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::MuteClick].ActionsPressed, "NumMuteClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::VolumeClick].ActionsPressed, "NumVolumeClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::PlayPauseClick].ActionsPressed, "NumPlayPauseClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::FullWindowClick].ActionsPressed, "NumFullWindowClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::ZoomClick].ActionsPressed, "NumZoomClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::SizeChanged].ActionsPressed, "NumSizeChanged"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::PositionSliderPressed].ActionsPressed, "NumPositionSliderPressed"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::ScrubbingMode].ActionsPressed, "NumScrubbingMode"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CastButtonClick].ActionsPressed, "NumCastingClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CCButtonClick].ActionsPressed, "NumClosedCaptionsClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CCTrackClick].ActionsPressed, "NumClosedCaptionsTrackSelect"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::MuteClick].ActionsFailed, "NumFailedMuteClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::VolumeClick].ActionsFailed, "NumFailedVolumeClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::PlayPauseClick].ActionsFailed, "NumFailedPlayPauseClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::FullWindowClick].ActionsFailed, "NumFailedFullWindowClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::ZoomClick].ActionsFailed, "NumFailedZoomClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::SizeChanged].ActionsFailed, "NumFailedSizeChanged"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::PositionSliderPressed].ActionsFailed, "NumFailedPositionSliderPressed"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::ScrubbingMode].ActionsFailed, "NumFailedScrubbingMode"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CastButtonClick].ActionsFailed, "NumFailedCastingClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CCButtonClick].ActionsFailed, "NumFailedClosedCaptionsClick"),
        TraceLoggingUInt32(m_actions[(int)MTCTelemetryType::CCTrackClick].ActionsFailed, "NumFailedClosedCaptionsTrackSelect"),
        TraceLoggingInt32(m_CurrentCCTrack, "CurrentCCTrack"),
        TraceLoggingUInt32(m_NumCCTurnedoff, "NumCCTurnedOff"),
        TraceLoggingUInt32(m_NumCCTrackChanged, "NumCCTrackChanged"),
        TraceLoggingBoolean(m_bIsControlPanelVisible, "IsControlPanelVisible"),
        TraceLoggingUInt32(m_NumControlPanelTransistions, "NumControlPanelTransitions"),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reset the value of the counters
//
//------------------------------------------------------------------------
void 
CAggMediaControlEvents::Clear()
{
    // Reset the values
    for (int i = 0; i < (int)MTCTelemetryType::MaxPressedActions; i++)
    {
        m_actions[i].Clear();
    }
    // Also reset the counters for events of interest
    m_NumControlPanelTransistions = 0;
    m_NumCCTurnedoff = 0;
    m_NumCCTrackChanged = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Telemetry Aggregation AddData function
//
//------------------------------------------------------------------------
void 
CAggMediaControlEvents::AddData(_In_ MTCTelemetryType type, _In_ const MTCTelemetryData& value)
{
    if (m_bNewData)
    {
        if ((GetTickCount64() - m_ulStart) > m_ulMS)
        {
            LogTelemetry();

            m_ulStart = GetTickCount64();
            const ULONGLONG ulNewPeriod = 2 * m_ulMS;
            const ULONGLONG ulMaxInterval = 300000;
            m_ulMS = (ulNewPeriod > ulMaxInterval) ? ulMaxInterval : ulNewPeriod;

            Clear();
            m_bNewData = false;
        }
    }

    // Capture data for all pressed actions
    if (type < MTCTelemetryType::MaxPressedActions && (int)type >= 0)
    {
        if (FAILED(value.errCode))
        {
            //store the last error code in case of a failure
            m_actions[(int)type].lastErrorcode = value.errCode;
            ++m_actions[(int)type].ActionsFailed;
        }
        //Update counter for each actions
        ++m_actions[(int)type].ActionsPressed;
    }

    switch (type)
    {
    case MTCTelemetryType::SetInstanceGuid:
        m_MEInstanceGuid = value.instanceGuid;
        break;
    case MTCTelemetryType::ControlPanelVisibility:
        if (m_bIsControlPanelVisible != value.isCPVisible)
        {
            m_bIsControlPanelVisible = value.isCPVisible;
            m_NumControlPanelTransistions++;
        }
        break;
    case MTCTelemetryType::CCButtonClick:
    case MTCTelemetryType::CCTrackClick:
        // Check the current track with the new track id
        if (m_CurrentCCTrack != value.trackId)
        {
            m_NumCCTrackChanged++;
            if (value.trackId == -1)
            {
                m_NumCCTurnedoff++;
            }
            // Save the current track information
            m_CurrentCCTrack = value.trackId;
        }
        break;
    default:
        break;
    }

    m_bNewData = true;
}
