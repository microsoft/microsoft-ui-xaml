// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <StableXbfIndexes.g.h>

// Used to identify which CustomWriterRuntimeData derivative should be used to serialize/deserialize an encoded custom
// binary blob from the XBFv2 stream
// When deserializing, the general pattern for using this enum should be as follows:
//
// // deserialize everything that was in version 1
// if (typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v1)
// {
//     // deserialize everything that was in version 2
//     if (typeIndex != CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v2)
//     {
//         // deserialize everything that was in version 3
//     }
// }
//
// Serialization should follow a similar pattern.
//
// Obviously, this pattern is only for the case where fields were exclusively added. A bespoke solution is required
// for other cases (e.g. removed fields).
// 
// When adding a new enum value, you will very likely need to also edit xcp\components\parser\inc\XbfVersioning.h.
// Specifically, adding a new TargetOSVersion constant to the Parser::Versioning::OSVersions namespace, as well as
// modifying the appropriate Get*SerializationVersion() helper method so that it knows about the new enum value.
enum class CustomWriterRuntimeDataTypeIndex : std::uint16_t
{
    // Legacy values derived from StableXbfTypeIndex
    DeferredElement_v1 = (std::uint16_t)Parser::StableXbfTypeIndex::DeferredElement,                        // 745; added in TH1
    ResourceDictionary_v1 = (std::uint16_t)Parser::StableXbfTypeIndex::ResourceDictionary,                  // 371; added in TH1
    VisualStateGroupCollection_v1 = (std::uint16_t)Parser::StableXbfTypeIndex::VisualStateGroupCollection,  // 420; added in TH1
    
    // New values should be added here. They must not conflict with legacy values above
    Unknown = 0,
    VisualStateGroupCollection_v2 = 1,      // added in TH1
    Style_v1 = 2,                           // added in TH1
    VisualStateGroupCollection_v3 = 3,      // added in TH1
    VisualStateGroupCollection_v4 = 4,      // added in TH1
    VisualStateGroupCollection_v5 = 5,      // added in TH1
    DeferredElement_v2 = 6,                 // added in TH1
    ResourceDictionary_v2 = 7,              // added in RS1
    Style_v2 = 8,                           // added in RS1; this was actually unnecessary, as the change that introduced it simply added a new flag to an existing bitfield which controls how to deserialize subsequent data
    DeferredElement_v3 = 9,                 // added in RS2
    ResourceDictionary_v3 = 10,             // added in RS2
};
