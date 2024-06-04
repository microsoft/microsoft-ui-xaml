// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StableXbfIndexes.g.h"

struct TargetOSVersion;
struct XamlTypeToken;
struct XamlPropertyToken;

namespace Parser
{
    // Return whether there is a StableXbfTypeIndex for the given KnownTypeIndex
    bool IsStableXbfType(const XamlTypeToken& token);
    // Return whether there is a StableXbfTypeIndex for the given KnownTypeIndex
    // This variant is intended to be used when generating XBF, since GenXbf's type table may
    // be a superset of the TPMV's type table, so we need to constrain the range of known
    // stable XBF indices
    bool IsStableXbfType(const XamlTypeToken& token, _In_ const TargetOSVersion& targetOSVersion);
    // Return whether there is a StableXbfPropertyIndex for the given KnownPropertyIndex
    bool IsStableXbfProperty(const XamlPropertyToken& token);
    // Return whether there is a StableXbfPropertyIndex for the given KnownPropertyIndex
    // This variant is intended to be used when generating XBF, since GenXbf's type table may
    // be a superset of the TPMV's type table, so we need to constrain the range of known
    // stable XBF indices
    bool IsStableXbfProperty(const XamlPropertyToken& token, _In_ const TargetOSVersion& targetOSVersion);

    // Return the StableXbfTypeIndex for a given KnownTypeIndex.
    KnownTypeIndex GetKnownTypeIndex(_In_ StableXbfTypeIndex index);
    StableXbfTypeIndex GetStableXbfTypeIndex(_In_ KnownTypeIndex index);
    // Return the StableXbfTypeIndex for a given KnownTypeIndex.
    // This variant is intended to be used when generating XBF, since GenXbf's type table may
    // be a superset of the TPMV's type table, so we need to constrain the range of known
    // stable XBF indices
    StableXbfTypeIndex GetStableXbfTypeIndex(_In_ KnownTypeIndex index, _In_ const TargetOSVersion& targetOSVersion);

    KnownPropertyIndex GetKnownPropertyIndex(_In_ StableXbfPropertyIndex index);
    // Return the StableXbfPropertyIndex for a given KnownPropertyIndex.
    StableXbfPropertyIndex GetStableXbfPropertyIndex(_In_ KnownPropertyIndex index);
    // Return the StableXbfPropertyIndex for a given KnownPropertyIndex.
    // This variant is intended to be used when generating XBF, since GenXbf's type table may
    // be a superset of the TPMV's type table, so we need to constrain the range of known
    // stable XBF indices
    StableXbfPropertyIndex GetStableXbfPropertyIndex(_In_ KnownPropertyIndex index, _In_ const TargetOSVersion& targetOSVersion);

    KnownEventIndex GetKnownEventIndex(_In_ StableEventIndex index);
    StableEventIndex GetStableEventIndex(_In_ KnownEventIndex index);
}