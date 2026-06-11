// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DebugOutput.h>

namespace DirectUI
{
    /* static */ void DebugOutput::LogXamlResourceReferenceErrorMessage(_In_ xstring_ptr message)
    {
        UNREFERENCED_PARAMETER(message);
    }

    /* static */ bool DebugOutput::IsLoggingForXamlResourceReferenceEnabled()
    {
        return false;
    }
}
