// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "Utils.h"
using namespace Platform;
using namespace Microsoft::People::Controls;

String^ StringUtil::FormatString(_In_ String^ formatString, ...)
{
    va_list pArgs;
    va_start(pArgs, formatString);

    // Allocate some space for the resulting string
    const DWORD formattedStringLength = 1024;
    std::auto_ptr<wchar_t> formattedString(new wchar_t[formattedStringLength]);

    // Format the string
    FormatMessage(
        FORMAT_MESSAGE_FROM_STRING,
        formatString->Data(),
        0,
        0,
        formattedString.get(),
        formattedStringLength,
        &pArgs);

    va_end(pArgs);

    return ref new String(formattedString.get());
}