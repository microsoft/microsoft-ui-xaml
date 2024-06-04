// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XbfMetadataApi.h"
#include "XamlTypeTokens.h"
#include "XamlPropertyToken.h"
#include "MetadataAPI.h"
#include "XbfVersioning.h"

namespace Parser
{
    extern const KnownTypeIndex c_aStableXbfTypeToKnownType[StableXbfTypeCount];
    extern const StableXbfTypeIndex c_aKnownTypeToStableXbfType[KnownTypeCount];

    extern const KnownPropertyIndex c_aStableXbfPropertyToKnownProperty[StableXbfPropertyCount];
    extern const StableXbfPropertyIndex c_aKnownPropertyToStableXbfProperty[KnownPropertyCount];

    extern const KnownEventIndex c_aStableEventToKnownEvent[StableEventCount];
    extern const StableEventIndex c_aKnownEventToStableEvent[KnownEventCount];

    bool IsStableXbfType(const XamlTypeToken& token)
    {
        return IsStableXbfType(token, Parser::Versioning::OSVersions::Latest());
    }

    bool IsStableXbfType(const XamlTypeToken& token, _In_ const TargetOSVersion& targetOSVersion)
    {
        return (token.GetProviderKind() == tpkNative || token.GetProviderKind() == tpkManaged) &&
            DirectUI::MetadataAPI::IsKnownIndex(token.GetHandle()) &&
            GetStableXbfTypeIndex(token.GetHandle(), targetOSVersion) != StableXbfTypeIndex::UnknownType;
    }

    bool IsStableXbfProperty(const XamlPropertyToken& token)
    {
        return IsStableXbfProperty(token, Parser::Versioning::OSVersions::Latest());
    }

    bool IsStableXbfProperty(const XamlPropertyToken& token, _In_ const TargetOSVersion& targetOSVersion)
    {
        return (token.GetProviderKind() == tpkNative || token.GetProviderKind() == tpkManaged) &&
            DirectUI::MetadataAPI::IsKnownIndex(token.GetHandle()) &&
            GetStableXbfPropertyIndex(token.GetHandle(), targetOSVersion) != StableXbfPropertyIndex::UnknownType_UnknownProperty;
    }

    KnownTypeIndex GetKnownTypeIndex(_In_ StableXbfTypeIndex index)
    {
        auto typedIndex = static_cast<std::underlying_type<StableXbfTypeIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < StableXbfTypeCount);
        return c_aStableXbfTypeToKnownType[typedIndex];
    }

    StableXbfTypeIndex GetStableXbfTypeIndex(_In_ KnownTypeIndex index)
    {
        return GetStableXbfTypeIndex(index, Parser::Versioning::OSVersions::Latest());
    }

    StableXbfTypeIndex GetStableXbfTypeIndex(_In_ KnownTypeIndex index, _In_ const TargetOSVersion& targetOSVersion)
    {
        auto typedIndex = static_cast<std::underlying_type<KnownTypeIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < KnownTypeCount);
        auto stableXbfIndex = c_aKnownTypeToStableXbfType[typedIndex];
        
        if (Parser::Versioning::GetStableXbfTypeIndexCountForTargetOSVersion(targetOSVersion) > static_cast<std::underlying_type<StableXbfTypeIndex>::type>(stableXbfIndex))
        {
            return stableXbfIndex;
        }
        else
        {
            return StableXbfTypeIndex::UnknownType;
        }
    }

    KnownPropertyIndex GetKnownPropertyIndex(_In_ StableXbfPropertyIndex index)
    {
        auto typedIndex = static_cast<std::underlying_type<StableXbfPropertyIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < StableXbfPropertyCount);
        return c_aStableXbfPropertyToKnownProperty[typedIndex];
    }

    StableXbfPropertyIndex GetStableXbfPropertyIndex(_In_ KnownPropertyIndex index)
    {
        return GetStableXbfPropertyIndex(index, Parser::Versioning::OSVersions::Latest());
    }

    StableXbfPropertyIndex GetStableXbfPropertyIndex(_In_ KnownPropertyIndex index, _In_ const TargetOSVersion& targetOSVersion)
    {
        auto typedIndex = static_cast<std::underlying_type<KnownPropertyIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < KnownPropertyCount);

        auto stableXbfIndex = c_aKnownPropertyToStableXbfProperty[typedIndex];

        if (Parser::Versioning::GetStableXbfPropertyIndexCountForTargetOSVersion(targetOSVersion) > static_cast<std::underlying_type<StableXbfPropertyIndex>::type>(stableXbfIndex))
        {
            return stableXbfIndex;
        }
        else
        {
            return StableXbfPropertyIndex::UnknownType_UnknownProperty;
        }
    }

    KnownEventIndex GetKnownEventIndex(_In_ StableEventIndex index)
    {
        auto typedIndex = static_cast<std::underlying_type<StableEventIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < StableEventCount);
        return c_aStableEventToKnownEvent[typedIndex];
    }

    StableEventIndex GetStableEventIndex(_In_ KnownEventIndex index)
    {
        auto typedIndex = static_cast<std::underlying_type<KnownEventIndex>::type>(index);
        ASSERT(0 <= typedIndex && typedIndex < KnownEventCount);
        return c_aKnownEventToStableEvent[typedIndex];
    }
}