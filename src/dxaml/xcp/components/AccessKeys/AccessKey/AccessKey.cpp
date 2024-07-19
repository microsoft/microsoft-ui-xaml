// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "AccessKey.h"

using namespace AccessKeys;

AKAccessKey::AKAccessKey(_In_ const wchar_t accessKey)
{
    (*this) = accessKey;
}

AKAccessKey::AKAccessKey(const std::wstring & accessKey)
{
    (*this) = accessKey;
}

AKAccessKey &AKAccessKey::operator=(const AKAccessKey & other)
{
    for (unsigned int i = 0; i < maxAccessKeyLength; i++)
    {
        accessKey[i] = other.accessKey[i];
    }
    accessKey[maxAccessKeyLength] = '\0';
    return *this;
}

AKAccessKey &AKAccessKey::operator=(const wchar_t other)
{
    accessKey[0] = other;

    for (unsigned int i = 1; i < maxAccessKeyLength; i++)
    {
        accessKey[i] = '\0';
    }

    accessKey[maxAccessKeyLength] = '\0';

    MakeAccessKeyUppercase();

    return *this;
}

AKAccessKey &AccessKeys::AKAccessKey::operator=(const std::wstring & other)
{
    for (unsigned int i = 0; i < maxAccessKeyLength; i++)
    {
        i < other.length() ? accessKey[i] = other[i] : accessKey[i] = '\0';
    }

    accessKey[maxAccessKeyLength] = '\0';

    MakeAccessKeyUppercase();

    return *this;
}

bool AccessKeys::AKAccessKey::operator==( const AKAccessKey& rhs) const
{
    for (unsigned int i = 0; i < maxAccessKeyLength; i++)
    {
        if (accessKey[i] != rhs.accessKey[i])
        {
            return false;
        }
    }

    return true;
}

void AKAccessKey::MakeAccessKeyUppercase()
{
    // First get the current locale.  
    LCID lcid = MAKELCID(::GetThreadUILanguage(), SORT_DEFAULT);

    // Per MSDN should use LCMapStringEx https://msdn.microsoft.com/en-us/library/windows/desktop/dd318702(v=vs.85).aspx.  Supported since winphone 8.1, win vista
    // For this we need a Locale name, not an ID, Let's get it.  
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    LCIDToLocaleName(lcid, localeName, LOCALE_NAME_MAX_LENGTH, 0);

    // At the time, this function is the same one StringUtil::CharUpperBuff uses to capitalize in richedit.  See the MSDN link above to get the full description
    const int result = LCMapStringEx(   localeName /*Locale to use*/,
                                        LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING /*Mapping options for the string*/,
                                        accessKey /*Source string buffer*/,
                                        maxAccessKeyLength /*Length of the buffer*/,
                                        accessKey /*Output string buffer*/,
                                        maxAccessKeyLength /*Length of output buffer*/,
                                        nullptr /*Reserved - must be null*/,
                                        nullptr /*Reserved - must be null*/,
                                        0 /*Reserved - must be 0*/);

    // When LCMapStringEX returns 0 - it was not successful, log a message for debugging.  Reading the MSDN doc, it does not seem the above call should ever return 0
    if (result == 0)
    {
        OutputDebugString(L"A case-insensitive version of the specified AccessKey String could not be found.  This component may not be accessible through AccessKeys.\n");
        IFCFAILFAST(E_FAIL);
    }
}

bool AKAccessKey::IsPartialMatch(const AKAccessKey& other) const
{
    for (unsigned int i = 0; i < maxAccessKeyLength; i++)
    {
        if (accessKey[i] == '\0' || accessKey[i] == other.accessKey[i])
        {
            continue;
        }
        else
        {
            return false;
        }
    }
    return true;
}

