// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Contents:  Event tracing helper class.

#include "evntprov.h"
#include "MUX-ETWEvents.h"

//+----------------------------------------------------------
//
// Class:      CXcpWinCodeTracer
//
// Implements: ITracingService interface
//
// Synopsis:   Provides a simple interface to produce
//             ETW (Event Trace for Windows) events.
//
//-----------------------------------------------------------
class CXcpWinCodeTracer final : public ITracingService
{

public:

    CXcpWinCodeTracer() {};
    ~CXcpWinCodeTracer();

    HRESULT Initialize();

    void TraceXcpEvent(_In_ XTraceMarkerType traceMarker, _In_opt_ const void* pData, _In_ XTraceType traceType, _In_ XTraceLevel level, _In_ XTraceSuite suite);

    XUINT8 IsTracingEnabled(_In_ XTraceLevel level, _In_ XTraceSuite suite);

    void GetTraceState(_Out_ XTraceLevel* level, _Out_ XTraceSuite* suite);

private:
    void WriteXcpEvent(_In_ XTraceMarkerType traceMarker, _In_opt_ const void* pData, _In_ XTraceType traceType);
};

