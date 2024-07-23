// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <string>

namespace AccessKeys {
    // Space for 6 characters.  In the past we had a max of three characters here, but this doesn't work well for
    // characters that are made up of two unicode code points (called "surrogate pairs").  We've raised the maximum to 6
    // so we can support 3 surrogate pairs.
    const unsigned int maxAccessKeyLength = 6; 
    // Encapsulates the representation of an AccessKey (effectively a vector of characters) and comparison methods.  Potentially accessors if needed.
    class AKAccessKey
    {
    public:
        AKAccessKey() = default;
        AKAccessKey(_In_ const wchar_t accessKey);
        AKAccessKey(_In_ const std::wstring& accessKey);

        AKAccessKey &operator= (const AKAccessKey &other);
        AKAccessKey &operator= (const wchar_t other);
        AKAccessKey &operator= (const std::wstring& other); // Copies first maxAccessKeyLength characters from other to this access key.  Should only be called from AKParser

        bool operator==(const AKAccessKey& rhs) const;
        bool operator!=(const AKAccessKey& rhs) const { return !operator==(rhs); };

        // True when the first non null characters of (this*) match the first characters of other.  If 2 AccessKeys are 
        // partial matches of each other, they are equal. 
        bool IsPartialMatch(const AKAccessKey& other) const;

        const wchar_t * const GetAccessKeyString() const { return accessKey; }
    private:
        void MakeAccessKeyUppercase();

        // Collection of characters parsed from the UI element owner's AutomationProperties.AccessKey field.
        // Declaring this with room for a null terminator.
        wchar_t accessKey[maxAccessKeyLength+1] {}; 
    };
}

namespace std {
    template <>
    struct hash<AccessKeys::AKAccessKey>
    {
        std::size_t operator()(const AccessKeys::AKAccessKey& k) const
        {
            return std::hash<std::wstring>()(std::wstring(k.GetAccessKeyString()));
        }
    };
}

