// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementRecord.h"

namespace DirectUI::WindowPlacementPersistence
{
    // Store-adapter formatting. These helpers perform no storage I/O and touch no window.

    // Base64 transport for the binary record. MaximumRecordBytes decoded bytes need this many
    // characters, so callers reject an oversized value before allocating a decode buffer.
    constexpr size_t MaximumEncodedCharacters = ((MaximumRecordBytes + 2) / 3) * 4; // 5464

    bool EncodeRecordText(const Record& record, std::u16string& text);
    DecodeResult DecodeRecordText(const char16_t* text, size_t length, Record& record);

    // Private container and value names. Ids are compared ordinally and are hashed as exact
    // UTF-16 code units. Hashing gives collision resistance, not confidentiality or access control.
    // Both reject an empty id and report platform hash failures as false.
    bool TryMakeApplicationContainerName(const char16_t* applicationId, size_t length, std::u16string& name);
    bool TryMakePlacementValueName(const char16_t* placementId, size_t length, std::u16string& name);
    bool TryMakePlacementGroupPropertyName(
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        std::u16string& name);
}
