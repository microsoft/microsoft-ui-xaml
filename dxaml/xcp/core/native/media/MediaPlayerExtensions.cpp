// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaPlayerExtensions.h"


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

