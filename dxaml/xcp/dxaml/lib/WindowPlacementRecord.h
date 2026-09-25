// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <guiddef.h>
#include <optional>
#include <string>
#include <vector>

namespace DirectUI::WindowPlacementPersistence
{
    // Private snapshot types. These are not the public API or native placement structures.
    struct Rect
    {
        int32_t X{};
        int32_t Y{};
        int32_t Width{};
        int32_t Height{};
    };

    enum class State
    {
        Normal,
        Maximized,
        Minimized,
        Snapped,
        MinimizedFromMaximized,
        MinimizedFromSnapped
    };

    struct Snapshot
    {
        Rect NormalRect;
        Rect WorkArea;
        int32_t Dpi{};
        State PlacementState{State::Normal};
        std::optional<Rect> SnapRect;
        std::u16string DisplayDeviceName;
        std::optional<GUID> VirtualDesktopId;
    };

    struct Record
    {
        Snapshot Placement;
        uint64_t SaveSequence{}; // Zero means unknown/oldest, not invalid placement.
    };

    enum class DecodeResult
    {
        Success,
        InvalidData,
        UnsupportedVersion
    };

    constexpr size_t MaximumRecordBytes = 4096;

    bool IsValid(const Snapshot& placement) noexcept;

    // Binary WPL1 codec, independent of base64, storage, HWNDs, and current topology.
    // Outputs are replaced only on success. Allocation failures are not invalid-data results.
    bool EncodeRecord(const Record& record, std::vector<uint8_t>& bytes);
    DecodeResult DecodeRecord(const uint8_t* bytes, size_t size, Record& record);
}
