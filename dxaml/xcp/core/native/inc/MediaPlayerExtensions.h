// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.media.h>

// Returns true when the MediaPlayer's underlying MediaEngine has already been closed/torn down.
// Probes with MediaPlaybackSession.PlaybackState (the recommended replacement for the deprecated
// MediaPlayer.CurrentState); if the session or its state can't be read, the engine is gone.
bool MediaPlayer_IsClosed(_In_opt_ wmp::IMediaPlayer* pMediaPlayer);

_Check_return_ HRESULT
MediaPlayer_GetCurrentPlaybackSession(
    _In_opt_ wmp::IMediaPlayer* pMediaPlayer,
    _Outptr_result_maybenull_ wmp::IMediaPlaybackSession** ppValue);

