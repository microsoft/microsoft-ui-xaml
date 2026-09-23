// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaPlayerExtensions.h"

// Returns true when the MediaPlayer's underlying MediaEngine has already been closed/torn down.
// We probe with MediaPlaybackSession.PlaybackState (the recommended replacement for the deprecated
// MediaPlayer.CurrentState). On a closed engine, obtaining the playback session or reading its state
// fails (e.g. E_ABORT), or the state reads back as MediaPlaybackState_None; both are treated as "closed".
bool MediaPlayer_IsClosed(_In_opt_ wmp::IMediaPlayer* pMediaPlayer)
{
    if (!pMediaPlayer)
    {
        return false;
    }

    ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer(pMediaPlayer);
    ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
    if (FAILED(spMediaPlayer.As(&spMediaPlayer3)))
    {
        return true;
    }

    ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
    if (FAILED(spMediaPlayer3->get_PlaybackSession(&spPlaybackSession)) || !spPlaybackSession)
    {
        return true;
    }

    // Two signals indicate a closed/torn-down engine: (1) we fail to read PlaybackState at all, or
    // (2) we read it successfully but it reports MediaPlaybackState_None (no live media engine backing the session).
    wmp::MediaPlaybackState playbackState = wmp::MediaPlaybackState_None;
    if (FAILED(spPlaybackSession->get_PlaybackState(&playbackState)))
    {
        return true;
    }

    return playbackState == wmp::MediaPlaybackState_None;
}

_Check_return_ HRESULT
MediaPlayer_GetCurrentPlaybackSession(_In_opt_ wmp::IMediaPlayer* pMediaPlayer, _Outptr_result_maybenull_ wmp::IMediaPlaybackSession** ppValue)
{
    *ppValue = nullptr;
    if (pMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer(pMediaPlayer);
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
        ctl::ComPtr<wmp::IMediaBreakManager> spBreakManager;
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        ctl::ComPtr<wmp::IMediaBreak> spMediaBreak;

        // A closed engine (e.g. a shared player closed during element teardown) means "no current session"; return
        // null instead of querying the torn-down engine and propagating the failure as a stowed-exception crash.
        if (MediaPlayer_IsClosed(pMediaPlayer))
        {
            TRACE(TraceAlways, L"MediaPlayer_GetCurrentPlaybackSession: MediaPlayer is closed; returning null session.");
            return S_OK;
        }

        IFC_RETURN(spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_BreakManager(&spBreakManager));
        IFC_RETURN(spBreakManager->get_CurrentBreak(&spMediaBreak));

        if (spMediaBreak)
        {
            IFC_RETURN(spBreakManager->get_PlaybackSession(&spPlaybackSession));
        }
        else
        {
            IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));
        }

        *ppValue = spPlaybackSession.Detach();
    }
    return S_OK;
}

