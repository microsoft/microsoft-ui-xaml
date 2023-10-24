// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Include this file instead of including dcompinternal.h directly.
// The CompInProc packages have some dependencies on symbols defined
// in the public SDK, but XAML's drop of the public SDK will always be
// behind CompInProc's view of it.  We define extra symbols here
// that are not yet a part of the SDK, but needed to build CompInProc.

// TODO http://task.ms/31623798 Remove extra typedefs in DCompExtras.h

typedef ULONG64 COMPOSITION_FRAME_ID;

typedef struct tagCOMPOSITION_FRAME_STATS
{
    UINT64 startTime;
    UINT64 targetTime;
    UINT64 framePeriod;
} COMPOSITION_FRAME_STATS;

typedef struct tagCOMPOSITION_TARGET_ID
{
    LUID adapterLuid;
    UINT vidPnSourceId;
    UINT uniqueId;
} COMPOSITION_TARGET_ID;

typedef struct tagCOMPOSITION_STATS
{
    UINT presentCount;
    UINT refreshCount;
    UINT virtualRefreshCount;
    UINT64 time;
} COMPOSITION_STATS;

typedef struct tagCOMPOSITION_TARGET_STATS
{
    UINT outstandingPresents;
    UINT64 presentTime;
    UINT64 vblankDuration;

    COMPOSITION_STATS presentedStats;
    COMPOSITION_STATS completedStats;
} COMPOSITION_TARGET_STATS;

