// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Performance marker definitions

#pragma once


#define PERF_WINDOWCLASSNAME   _T("xcpperfutil")
#define PERF_WINDOWNAME        _T("xcpperfutil window")
#define BROWSERCONTROL_WINDOWCLASSNAME   _T("BrowserControl")


// event for performance data. WM_APP->WM_APP+3 are used by the listener application
#define WM_XCPPERFDATA WM_APP+4

#define XCP_PERF_LOG(eventID)     \
    if (gps.IsValid())\
    { \
        PerfMarkerType perfMarkerType = GetPALDebuggingServices()->PerfMarkersEnabled(); \
        if (perfMarkerType != PerfMarkersDisabled) \
        { \
            (void)GetPALDebuggingServices()->XcpPerfMarker(eventID); \
        } \
    }

#define XCP_PERF_LOG_STRING_INFO(eventID, Info)     \
    if (gps.IsValid())\
    { \
        PerfMarkerType perfMarkerType = GetPALDebuggingServices()->PerfMarkersEnabled(); \
        if (perfMarkerType != PerfMarkersDisabled) \
        { \
            (void)GetPALDebuggingServices()->XcpPerfMarkerStringInfo(eventID, Info); \
        } \
    }

#define XCP_PERF_LOG_DWORD_INFO(eventID, Info)     \
    if (gps.IsValid())\
    { \
        PerfMarkerType perfMarkerType = GetPALDebuggingServices()->PerfMarkersEnabled(); \
        if (perfMarkerType != PerfMarkersDisabled) \
        { \
            (void)GetPALDebuggingServices()->XcpPerfMarkerDwordInfo(eventID, Info); \
        } \
    }

//
//  Each marker is a 64-bit chunk of data, broken into blocks:
//
//  |----------|---|--------------|-------------------------------|
//      12       4         16                    32
//
//  bits    purpose             expected values
//  12      type of data        event, begin log, end log, service error, service
//                              allocation of additional memory
//  4       binary source ID    agcore.dll, agctrl.dll, agview.exe, agwin.dll,
//                              agmac.dll, test executable, etc. (will we track
//                              more than 15 binaries?)
//  16      currently reserved  0x0000
//  32      type-specific data  defined per event type.
//
//  The first block tells you what it is. If it�s an event, then the next 64 bits are a time
//  marker from the high performance counter. This also means that event markers will have up
//  to 128 bits (16 bytes) of data as part of the notification.
//

const XUINT64 XCP_PMK_MASK32_0 = 0x00000000FFFFFFFFLL;
const XUINT64 XCP_PMK_MASK64_33 = (const XUINT64)0xFFFFFFFF00000000LL;


//------------- bits 64 - 53 --------------------------------------------------
//
//      NOTE: this is a bitfield for easy query

//
//  data type: event (performance counter)
//
const XUINT64 XCP_PMK_EVENT = 0x0010000000000000LL;

//
//  data type: frequency
//
const XUINT64 XCP_PMK_FREQ = 0x0020000000000000LL;

//
//  data type: begin log
//
const XUINT64 XCP_PMK_BEGINLOG = 0x0040000000000000LL;

//
//  data type: end log
//
const XUINT64 XCP_PMK_ENDLOG = 0x0080000000000000LL;

//
//  data type: general error
//
const XUINT64 XCP_PMK_ERROR = 0x0100000000000000LL;


//------------- bits 52 - 49 --------------------------------------------------
//
//


//
//  source id: agcore.dll
//
const XUINT64 XCP_PMK_XCPCORE = 0x0001000000000000LL;

//
//  source id: agctrl.dll
//
const XUINT64 XCP_PMK_XCPCTRL = 0x0002000000000000LL;

//
//  source id: agwin.dll
//
const XUINT64 XCP_PMK_XCPWIN = 0x0003000000000000LL;

//
//  source id: agmac.dll
//
const XUINT64 XCP_PMK_XCPMAC = 0x0004000000000000LL;

//
//  source id: agview.exe
//
const XUINT64 XCP_PMK_XCPVIEW = 0x0005000000000000LL;

//
//  source id: test app
//
const XUINT64 XCP_PMK_TESTAPP = 0x0006000000000000LL;



//------------- bits 48 - 33 --------------------------------------------------
//
//      NOTE: currently unassigned


//------------- bits 32 - 1  --------------------------------------------------
//
//      NOTE: this is specific to the type of data
//
#include "JoltPerf.h"