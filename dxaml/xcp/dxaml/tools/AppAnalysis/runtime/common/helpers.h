// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "microsoft.diagnostics.appanalysis.internal.h"
#include <memory>
#include "wil_resource.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "NamespaceAliases.h"
#include "ResourceString.h"

namespace AppAnalysisHelpers
{
    static int CompareStrings(HSTRING str1, HSTRING str2)
    {
        int compareResult = 0;
        WindowsCompareStringOrdinal(str1, str2, &compareResult);
        return compareResult;
    }

    // right now this is pretty simple, but we may want to filter out an apps
    // version of Generic.xaml for those apps that retemplate our controls
    static bool IsGenericXaml(const appanalysis::SourceInfo& sourceInfo)
    {
        return ((AppAnalysisHelpers::CompareStrings(sourceInfo.FileName, StringRef(L"ms-resource:///Files/Microsoft.UI.Xaml;component/themes/generic.xaml")) == 0) ||
                (AppAnalysisHelpers::CompareStrings(sourceInfo.FileName, StringRef(L"ms-resource:///Files/Microsoft.UI.Xaml;component/themes/themeresources.xbf")) == 0));
    }

    static HRESULT ToString(int id, HSTRING *outString)
    {
        wchar_t string[256] = { 0 };
        IFC_RETURN(StringCchPrintf(string, _countof(string), L"%d", id));

        size_t size = 0;
        IFC_RETURN(StringCchLength(string, _countof(string), &size));
        IFC_RETURN(WindowsCreateString(string, static_cast<UINT32>(size), outString));

        return S_OK;
    }

    struct GuidMapComparer
    {
        bool operator()(const GUID& left, const GUID& right) const
        {
            return memcmp(&left, &right, sizeof(right)) < 0;
        }
    };

    struct GuidSetComparer
    {
        bool operator()(const GUID& left, const GUID& right) const
        {
            return (0 < IsEqualGUID(left, right));
        }
    };

    struct GuidSetHash
    {
        int operator()(_In_ const GUID& guid) const
        {
            unsigned int const* const p = reinterpret_cast<unsigned int const*>(&guid);
            unsigned int hash = 0;

            for (unsigned i = 0; i != 4; i++)
            {
                hash += p[i];
                hash += hash << 10;
                hash ^= hash >> 6;
            }

            // A bunch of magic numbers here
            hash += hash << 3;
            hash ^= hash >> 11;
            hash += hash << 15;
            return hash;
        }
    };

    using GuidSet = std::unordered_set<GUID, AppAnalysisHelpers::GuidSetHash, AppAnalysisHelpers::GuidSetComparer>;


    struct SourceInfoComparer
    {
        // The comparison operator for STL containers should return true if the 
        // object goes before the element in the container, and false if after.
        // Under this convention, we'll assume that if a filename is null, it goes
        // to the left of the filename that isn't null.
        bool operator()(const appanalysis::SourceInfo& left, const appanalysis::SourceInfo& right) const
        {
            // the only way both filenames in this comparison will be null is if a SourceInfo
            // element with a null filename is placed in the container. This shouldn't be allowed
            // so we'll ASSERT here to catch those scenarios.

            ASSERT(left.FileName != nullptr || right.FileName != nullptr);

            // compare the filenames
            int fileDiff = AppAnalysisHelpers::CompareStrings(left.FileName, right.FileName);

            // if the files are the same, then we need to be smarter
            if (fileDiff == 0)
            {
                int lineDiff = left.LineNumber - right.LineNumber;
                // if on the same line (unlikely) then check column
                if (lineDiff == 0)
                {
                    return ((left.ColumnNumber - right.ColumnNumber) < 0);
                }
                else
                {
                    return lineDiff < 0;
                }
            }
            else
            {
                return fileDiff < 0;
            }
        }
    };

    template <typename Implements, typename Queried>
    static HRESULT As(_In_opt_ Implements* obj, _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<Queried>> queried)
    {
        Queried** pInterface = queried.ReleaseAndGetAddressOf();
        if (obj)
        {
            IFC_RETURN(obj->QueryInterface<Queried>(pInterface));
        }

        return S_OK;
    }
}
