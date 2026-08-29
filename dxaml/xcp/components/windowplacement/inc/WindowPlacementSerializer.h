// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>
#include "WindowPlacementData.h"

// Serialize a WindowPlacementData to the WPL1 wire format and parse it back. WinUI owns this
// format; see docs/design-notes/Window-PlacementPersistence.md sections 3.3, 3.4.
//
// The serializer never throws and the parser is fail-safe: any malformed, truncated, or
// unknown-major blob is reported as "no saved placement" rather than a partial result. This keeps
// restore from ever applying incomplete state and lets a caller fall back to normal first-show
// placement.

namespace WindowPlacement
{
    // Serialize a placement to the raw WPL1 binary envelope (not Base64).
    //
    // Only known flag bits are written. Optional fields are emitted only when their presence flag
    // is set. Tags are written once, in ascending order, so a given placement produces a byte-for-
    // byte canonical blob suitable for a golden test.
    //
    // Returns an empty vector when the placement has no normal rectangle (nothing to persist).
    std::vector<uint8_t> SerializePlacement(const WindowPlacementData& data);

    // Serialize a placement and Base64-encode it into the opaque string stored in LocalSettings.
    // Returns an empty string when there is nothing to persist.
    std::wstring SerializePlacementToBase64(const WindowPlacementData& data);

    // Parse a raw WPL1 binary envelope. Returns true and fills 'data' only for a valid blob that
    // contains at least TAG_NORMAL_RECT. Returns false (and leaves 'data' default) for any missing
    // value, truncation, bad length, out-of-range coordinate, unknown major version, or oversized
    // blob.
    bool TryParsePlacement(const uint8_t* bytes, size_t length, WindowPlacementData& data);

    inline bool TryParsePlacement(const std::vector<uint8_t>& bytes, WindowPlacementData& data)
    {
        return TryParsePlacement(bytes.data(), bytes.size(), data);
    }

    // Base64-decode a LocalSettings string and parse it. Returns false for invalid Base64 as well
    // as any parse failure.
    bool TryParsePlacementFromBase64(const std::wstring& base64, WindowPlacementData& data);
}
