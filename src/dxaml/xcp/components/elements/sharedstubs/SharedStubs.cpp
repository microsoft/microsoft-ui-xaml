// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <corep.h>
#include <set>
#include <ErrorHelper.h>

long __stdcall DirectUI::ErrorHelper::OriginateErrorUsingResourceID(long hr,unsigned int resID) { return hr; }
_Check_return_ long __stdcall DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(long hr,unsigned int resID, _In_ class xstring_ptr_view const &str, const WCHAR* param2) { return hr; }

extern "C"
void MicrosoftTelemetryAssertTriggeredArgs(const char *OriginatingBinary, ULONG BucketArg1, ULONG BucketArg2)
{
}