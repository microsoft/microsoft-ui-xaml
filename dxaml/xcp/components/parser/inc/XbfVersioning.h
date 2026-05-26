// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeDataTypeIndex.h>
#include <XamlBinaryMetadata.h>
#include <StableXbfIndexes.g.h>

struct TargetOSVersion
{
    TargetOSVersion(unsigned short major, unsigned short minor, unsigned short build, unsigned short revision)
    {
        this->major = major;
        this->minor = minor;
        this->build = build;
        this->revision = revision;
    }

    bool operator==(const TargetOSVersion& rhs) const
    {
        return this->version == rhs.version;
    }
    bool operator!=(const TargetOSVersion& rhs) const
    {
        return !operator==(rhs);
    }

    bool operator<(const TargetOSVersion& rhs) const
    {
        if (this->major > rhs.major)
        {
            return false;
        }
        else if (this->major < rhs.major)
        {
            return true;
        }
        else
        {
            if (this->minor > rhs.minor)
            {
                return false;
            }
            else if (this->minor < rhs.minor)
            {
                return true;
            }
            else
            {
                if (this->build > rhs.build)
                {
                    return false;
                }
                else if (this->build < rhs.build)
                {
                    return true;
                }
                else
                {
                    if (this->revision > rhs.revision)
                    {
                        return false;
                    }
                    else if (this->revision < rhs.revision)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
    }

    bool operator>(const TargetOSVersion& rhs) const
    {
        return !operator<(rhs) && operator!=(rhs);
    }

    bool operator<=(const TargetOSVersion& rhs) const
    {
        return !operator>(rhs);
    }

    bool operator>=(const TargetOSVersion& rhs) const
    {
        return !operator<(rhs);
    }

    union
    {
        unsigned long long version;
        struct
        {
            unsigned short major;
            unsigned short minor;
            unsigned short build;
            unsigned short revision;
        };
    };
};

namespace Parser { namespace Versioning {

namespace OSVersions
{
    // It is not necessary to update this list every single time a new version of
    // the OS is released. Instead, it only needs to be updated when a new version
    // of significance is released (i.e. the release contains a new CustomWriterRuntimeData
    // format version or even a new XBF format)
    static const TargetOSVersion WINBLUE = { 6, 3, 9600, 0 };
    static const TargetOSVersion WIN10_TH1 = { 10, 0, 10240, 0 };
    static const TargetOSVersion WIN10_TH2 = { 10, 0, 10586, 0 };
    static const TargetOSVersion WIN10_RS1 = { 10, 0, 14393, 0 };
    static const TargetOSVersion WIN10_RS2 = { 10, 0, 15063, 0 };
    static const TargetOSVersion WIN10_RS3 = { 10, 0, 16299, 0 };
    static const TargetOSVersion WIN10_RS4 = { 10, 0, 17134, 0 };
    static const TargetOSVersion WIN10_RS5 = { 10, 0, 17763, 0 };
    static const TargetOSVersion WIN10_19H1 = { 10, 0, 19000, 0 };  // VSO:20462629: update this once we have official 19H1 version number

    // Update this when adding a new version
    static const TargetOSVersion& Latest() { return WIN10_19H1; }
};

// Returns the version of XBF to use for the given target OS
static const XamlBinaryFileVersion& GetXBFVersion(const TargetOSVersion& /* osVersion */)
{
    return Parser::XbfV2_1;
}

// Returns the CustomWriterRuntimeDataTypeIndex to use when serializing DeferredElementCustomWriterRuntimeData for the given target OS
static CustomWriterRuntimeDataTypeIndex GetDeferredElementSerializationVersion(const TargetOSVersion& /* osVersion */)
{
    return CustomWriterRuntimeDataTypeIndex::DeferredElement_v3;
}

// Returns the CustomWriterRuntimeDataTypeIndex to use when serializing ResourceDictionaryCustomWriterRuntimeData for the given target OS
static CustomWriterRuntimeDataTypeIndex GetResourceDictionarySerializationVersion(const TargetOSVersion& /* osVersion */)
{
    return CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3;
}

// Returns the CustomWriterRuntimeDataTypeIndex to use when serializing StyleCustomWriterRuntimeData for the given target OS
static CustomWriterRuntimeDataTypeIndex GetStyleSerializationVersion(const TargetOSVersion& /* osVersion */)
{
    return CustomWriterRuntimeDataTypeIndex::Style_v3;
}

// Returns the CustomWriterRuntimeDataTypeIndex to use when serializing VisualStateGroupCollectionCustomWriterRuntimeData for the given target OS
static CustomWriterRuntimeDataTypeIndex GetVisualStateGroupCollectionSerializationVersion(const TargetOSVersion& /* osVersion */)
{
    return CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5;
}

static std::underlying_type_t<::Parser::StableXbfTypeIndex> GetStableXbfTypeIndexCountForTargetOSVersion(const TargetOSVersion& osVersion)
{
    return StableXbfTypeCount;
}

static std::underlying_type_t<StableXbfPropertyIndex> GetStableXbfPropertyIndexCountForTargetOSVersion(const TargetOSVersion& /* osVersion */)
{
    return StableXbfPropertyCount;
}

} };