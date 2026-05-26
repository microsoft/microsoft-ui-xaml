// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XcpAllocationDebug.h"

#if XCP_MONITOR

IThreadMonitor* XcpDebugGetThreadMonitor()
{
    return nullptr;
}

bool XcpDebugCaptureStack(XUINT32 *, XUINT64 **)
{
    return false;
}

bool XcpDebugIsStressModeEnabled()
{
    return false;
}

void XcpDebugTraceLeakFullRefs(DebugMemoryCheckBlock *)
{
}

void XcpDebugTrace(XUINT32, DebugAllocationClass, void *, size_t )
{
}

bool XcpDebugTrace(XUINT32, const XCHAR *, XINT32, XINT32, const XCHAR *, const XCHAR *, ...)
{
    return true;
}

#endif // XCP_MONITOR
