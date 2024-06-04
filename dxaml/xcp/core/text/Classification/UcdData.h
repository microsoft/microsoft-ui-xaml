// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum UcdProperty;

// Ucd functions
bool    UcdInitialize();

// The binary data starts with a header and is followed by a variable-sized
// array of property directory entries.
//
// The entries MUST be sorted first on the property field, then on the plane
// field.

struct UcdPropertyInfo
{
    int32_t     prop;            // property
    uint32_t    offset;          // offset from the start of the file
};

#pragma warning(push)
#pragma warning(disable: 4200)      // zero-sized array in struct/union

struct UcdFileHeader
{
    uint32_t        properties_count;
    UcdPropertyInfo properties[];
};

// The UCD binary data is organized as a multi-level lookup table.  Each level
// of the table references ChildBlockLevels child levels.
static const uint32_t ChildBlockBits = 6;
static const uint32_t ChildBlockLevels = (1 << ChildBlockBits);

#pragma warning(pop)
