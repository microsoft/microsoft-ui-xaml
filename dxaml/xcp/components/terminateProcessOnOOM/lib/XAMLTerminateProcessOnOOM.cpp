// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.h>
#include "XAMLTerminateProcessOnOOM.h"

void XAMLTerminateProcessOnMemoryExhaustion(_In_ size_t cbFailedAllocationSize)
{
    EXCEPTION_RECORD ExceptionRecord = {};

    ExceptionRecord.ExceptionCode = 0xC00001ADL; //STATUS_FATAL_MEMORY_EXHAUSTION;
    ExceptionRecord.ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
    RaiseFailFastException(&ExceptionRecord, NULL, 0);

    ASSERT(FALSE); // The above call should never return
}