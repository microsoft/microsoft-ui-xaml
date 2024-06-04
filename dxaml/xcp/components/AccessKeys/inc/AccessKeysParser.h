// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AccessKey.h"

namespace AccessKeys {
    class AKParser
    {
    public:
        // Attempts to AutomationProperties.AccessKey property field the input accessString into an array of wchar_t (parsedOutputBuffer)
        // Returns true with valid access keys, and false otherwise.  Writes the parsed output into the output buffer.
        static bool TryParseAccessKey(_In_ const std::wstring& accessString, _Inout_ AKAccessKey& accessKey);

        // Returns true if the passed accessString contains a valid mnemonics access key
        static bool IsValidAccessKey(_In_ const std::wstring& accessString);

    private:
        static bool ContainsInvalidSubstring(_In_ const std::wstring& accessString);
    };

}
