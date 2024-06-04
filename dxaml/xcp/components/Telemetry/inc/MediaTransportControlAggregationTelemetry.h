// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class MTCTelemetryType
{
    // Enumerate all ButtonPressActions from 0-MaxPressedActions
    MuteClick = 0,
    VolumeClick,
    PlayPauseClick,
    FullWindowClick,
    ZoomClick,
    SizeChanged,
    PositionSliderPressed,
    ScrubbingMode,
    CastButtonClick,
    CCButtonClick,
    CCTrackClick,

    //Total count of ButtonPressedActions
    MaxPressedActions,

    // Additional datapoints of interest
    SetInstanceGuid,
    ControlPanelVisibility
};

union MTCTelemetryData
{
    struct
    {
        HRESULT errCode;
        int trackId;
    };
    GUID instanceGuid;
    BOOLEAN isCPVisible;
    MTCTelemetryData()
    {
        ZeroMemory(this, sizeof(MTCTelemetryData));
    }
};

//MediaControlActions Aggregation Variable
struct MediaControlActions
{
    unsigned int ActionsPressed;
    unsigned int ActionsFailed;
    HRESULT lastErrorcode;

    MediaControlActions() :
        ActionsPressed(0)
      , ActionsFailed(0)
      , lastErrorcode(S_OK)
    {
    }
    void Clear()
    {
        ActionsPressed = 0;
        ActionsFailed = 0;
        lastErrorcode = S_OK;
    }
};

class CAggMediaControlEvents
{
    private:
        MediaControlActions m_actions[static_cast<size_t>(MTCTelemetryType::MaxPressedActions)];
        GUID m_MEInstanceGuid; // Id used by Media Engine to correlate telemetry events
        BOOLEAN m_bIsControlPanelVisible; // Current state whether MTC is visible or not
        unsigned int m_NumControlPanelTransistions; // # of times MTC has transitions between show/hide
        int m_CurrentCCTrack; // Current closed caption track selected (-1: No track, >0: track selected)
        unsigned int m_NumCCTurnedoff; // # of times CC has been turned off
        unsigned int m_NumCCTrackChanged; // # of times CC track has changed (includes turning off)
        bool m_bNewData; // True if there is new telemetry to log
        ULONGLONG m_ulMS, // Each time telemetry is logged, we back off by 2^i * m_ulMs where i is the iteration
                          // (unless we have already reached the maximum back off)
                  m_ulStart; // When telemetry was last logged
    public:
        CAggMediaControlEvents();
        ~CAggMediaControlEvents();

        void AddData(_In_ MTCTelemetryType type, _In_ const MTCTelemetryData& value);
        void LogTelemetry();
        void Clear();
};
