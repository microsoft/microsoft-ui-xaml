// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.media.h>

_Check_return_ HRESULT
MediaPlayer_GetCurrentPlaybackSession(
    _In_opt_ wmp::IMediaPlayer* pMediaPlayer,
    _Outptr_result_maybenull_ wmp::IMediaPlaybackSession** ppValue);

