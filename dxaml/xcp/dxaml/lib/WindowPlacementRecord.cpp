// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementRecord.h"

#include <limits>
#include <utility>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        constexpr uint16_t NormalRectTag = 0x0001;
        constexpr uint16_t WorkAreaTag = 0x0002;
        constexpr uint16_t ArrangeRectTag = 0x0003;
        constexpr uint16_t DpiTag = 0x0010;
        constexpr uint16_t ShowCommandTag = 0x0011;
        constexpr uint16_t FlagsTag = 0x0012;
        constexpr uint16_t DeviceNameTag = 0x0020;
        constexpr uint16_t VirtualDesktopTag = 0x0021;
        constexpr uint16_t SaveSequenceTag = 0x0030;
        constexpr uint32_t RestoreMaximized = 0x0001;
        constexpr uint32_t Snapped = 0x0002;
        constexpr uint32_t RestoreSnapped = 0x0020;
        constexpr int64_t MaxCoordinate = (std::numeric_limits<int32_t>::max)();

        bool IsValidRect(const Rect& rect) noexcept
        {
            return rect.Width > 0 && rect.Height > 0 &&
                int64_t{rect.X} + rect.Width <= MaxCoordinate &&
                int64_t{rect.Y} + rect.Height <= MaxCoordinate;
        }

        bool IsSnapped(State state) noexcept
        {
            return state == State::Snapped || state == State::MinimizedFromSnapped;
        }

        bool IsEmptyGuid(const GUID& id) noexcept
        {
            if (id.Data1 || id.Data2 || id.Data3) return false;
            for (auto value : id.Data4)
            {
                if (value) return false;
            }
            return true;
        }

        // Callers establish the field's length before reading. Never read packed native structs.
        uint64_t ReadUnsigned(const uint8_t* bytes, size_t count) noexcept
        {
            uint64_t value = 0;
            for (size_t i = 0; i < count; ++i)
            {
                value |= uint64_t{bytes[i]} << (i * 8);
            }
            return value;
        }

        int32_t ReadCoordinate(const uint8_t* bytes) noexcept
        {
            const auto value = ReadUnsigned(bytes, 4);
            // Avoid an implementation-defined out-of-range unsigned-to-signed conversion.
            return static_cast<int32_t>(value <= MaxCoordinate ?
                static_cast<int64_t>(value) : static_cast<int64_t>(value) - 0x100000000LL);
        }

        bool ReadRect(const uint8_t* bytes, Rect& rect) noexcept
        {
            const auto left = ReadCoordinate(bytes);
            const auto top = ReadCoordinate(bytes + 4);
            const auto width = int64_t{ReadCoordinate(bytes + 8)} - left;
            const auto height = int64_t{ReadCoordinate(bytes + 12)} - top;
            if (width <= 0 || height <= 0 || width > MaxCoordinate || height > MaxCoordinate)
            {
                return false;
            }
            rect = {left, top, static_cast<int32_t>(width), static_cast<int32_t>(height)};
            return true;
        }

        void WriteUnsigned(std::vector<uint8_t>& bytes, uint64_t value, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
            }
        }

        void WriteTag(std::vector<uint8_t>& bytes, uint16_t tag, uint16_t length)
        {
            WriteUnsigned(bytes, tag, 2);
            WriteUnsigned(bytes, length, 2);
        }

        void WriteRect(std::vector<uint8_t>& bytes, uint16_t tag, const Rect& rect)
        {
            WriteTag(bytes, tag, 16);
            WriteUnsigned(bytes, static_cast<uint32_t>(rect.X), 4);
            WriteUnsigned(bytes, static_cast<uint32_t>(rect.Y), 4);
            WriteUnsigned(bytes, static_cast<uint32_t>(int64_t{rect.X} + rect.Width), 4);
            WriteUnsigned(bytes, static_cast<uint32_t>(int64_t{rect.Y} + rect.Height), 4);
        }

        bool ReadState(uint32_t command, uint32_t flags, State& state) noexcept
        {
            // Display aliases only; unknown/non-displaying commands default to normal.
            switch (command)
            {
            case 3: state = State::Maximized; break;
            case 2: case 6: case 7: case 11: state = State::Minimized; break;
            default: state = State::Normal; break;
            }

            switch (flags & (RestoreMaximized | Snapped | RestoreSnapped))
            {
            case 0: return true;
            case RestoreMaximized:
                if (state != State::Minimized) return false;
                state = State::MinimizedFromMaximized;
                return true;
            case Snapped:
                if (state != State::Normal) return false;
                state = State::Snapped;
                return true;
            case RestoreSnapped:
                if (state != State::Minimized) return false;
                state = State::MinimizedFromSnapped;
                return true;
            default: return false;
            }
        }
    }

    bool IsValid(const Snapshot& placement) noexcept
    {
        if (!IsValidRect(placement.NormalRect) || !IsValidRect(placement.WorkArea) ||
            (placement.SnapRect && !IsValidRect(*placement.SnapRect)) ||
            placement.Dpi < 96 || placement.DisplayDeviceName.size() > 31 ||
            placement.DisplayDeviceName.find(u'\0') != std::u16string::npos)
        {
            return false;
        }

        const auto& normal = placement.NormalRect;
        const auto& work = placement.WorkArea;
        if (int64_t{normal.X} + normal.Width <= work.X || int64_t{work.X} + work.Width <= normal.X ||
            int64_t{normal.Y} + normal.Height <= work.Y || int64_t{work.Y} + work.Height <= normal.Y)
        {
            return false;
        }

        switch (placement.PlacementState)
        {
        case State::Normal: case State::Maximized: case State::Minimized:
        case State::MinimizedFromMaximized:
            return true;
        case State::Snapped: case State::MinimizedFromSnapped:
            return placement.SnapRect.has_value();
        default:
            return false;
        }
    }

    bool EncodeRecord(const Record& record, std::vector<uint8_t>& bytes)
    {
        const auto& placement = record.Placement;
        if (!IsValid(placement)) return false;

        std::vector<uint8_t> result{'W', 'P', 'L', '1', 1, 0, 0, 0, 0, 0, 0, 0};
        result.reserve(194);
        WriteRect(result, NormalRectTag, placement.NormalRect);
        WriteRect(result, WorkAreaTag, placement.WorkArea);
        if (IsSnapped(placement.PlacementState))
        {
            WriteRect(result, ArrangeRectTag, *placement.SnapRect);
        }
        WriteTag(result, DpiTag, 4);
        WriteUnsigned(result, static_cast<uint32_t>(placement.Dpi), 4);

        uint32_t command = 1;
        uint32_t flags = 0;
        switch (placement.PlacementState)
        {
        case State::Maximized: command = 3; break;
        case State::Minimized: command = 2; break;
        case State::Snapped: flags = Snapped; break;
        case State::MinimizedFromMaximized: command = 2; flags = RestoreMaximized; break;
        case State::MinimizedFromSnapped: command = 2; flags = RestoreSnapped; break;
        default: break; // IsValid already checked the state.
        }
        WriteTag(result, ShowCommandTag, 4);
        WriteUnsigned(result, command, 4);
        WriteTag(result, FlagsTag, 4);
        WriteUnsigned(result, flags, 4);

        if (!placement.DisplayDeviceName.empty())
        {
            WriteTag(result, DeviceNameTag, static_cast<uint16_t>(placement.DisplayDeviceName.size() * 2));
            for (auto unit : placement.DisplayDeviceName) WriteUnsigned(result, unit, 2);
        }
        if (placement.VirtualDesktopId && !IsEmptyGuid(*placement.VirtualDesktopId))
        {
            const auto& id = *placement.VirtualDesktopId;
            WriteTag(result, VirtualDesktopTag, 16);
            WriteUnsigned(result, id.Data1, 4);
            WriteUnsigned(result, id.Data2, 2);
            WriteUnsigned(result, id.Data3, 2);
            for (auto value : id.Data4) result.push_back(value);
        }
        if (record.SaveSequence != 0)
        {
            WriteTag(result, SaveSequenceTag, 8);
            WriteUnsigned(result, record.SaveSequence, 8);
        }
        for (size_t i = 0; i < 4; ++i) result[8 + i] = static_cast<uint8_t>(result.size() >> (i * 8));
        bytes = std::move(result);
        return true;
    }

    DecodeResult DecodeRecord(const uint8_t* bytes, size_t size, Record& record)
    {
        if (!bytes || size < 12 || size > MaximumRecordBytes ||
            bytes[0] != 'W' || bytes[1] != 'P' || bytes[2] != 'L' || bytes[3] != '1' ||
            ReadUnsigned(bytes + 8, 4) != size)
        {
            return DecodeResult::InvalidData;
        }
        if (ReadUnsigned(bytes + 4, 2) != 1) return DecodeResult::UnsupportedVersion;
        // Higher minor versions are safe: unknown TLVs are bounded and skipped.
        Record result;
        bool hasNormal = false, hasWorkArea = false, hasDpi = false;
        uint32_t command = 1, flags = 0;
        for (size_t offset = 12; offset < size;)
        {
            if (size - offset < 4) return DecodeResult::InvalidData;
            const auto tag = ReadUnsigned(bytes + offset, 2);
            const auto length = static_cast<size_t>(ReadUnsigned(bytes + offset + 2, 2));
            offset += 4;
            if (length > size - offset) return DecodeResult::InvalidData;
            const auto value = bytes + offset;
            auto& placement = result.Placement;

            // Validate every occurrence before last-one-wins assignment, including unused snap data.
            switch (tag)
            {
            case NormalRectTag: case WorkAreaTag: case ArrangeRectTag:
            {
                Rect rect;
                if (length != 16 || !ReadRect(value, rect)) return DecodeResult::InvalidData;
                if (tag == NormalRectTag) { placement.NormalRect = rect; hasNormal = true; }
                else if (tag == WorkAreaTag) { placement.WorkArea = rect; hasWorkArea = true; }
                else placement.SnapRect = rect;
                break;
            }
            case DpiTag:
            {
                if (length != 4) return DecodeResult::InvalidData;
                const auto dpi = ReadUnsigned(value, 4);
                if (dpi < 96 || dpi > MaxCoordinate) return DecodeResult::InvalidData;
                placement.Dpi = static_cast<int32_t>(dpi);
                hasDpi = true;
                break;
            }
            case ShowCommandTag: case FlagsTag:
                if (length != 4) return DecodeResult::InvalidData;
                if (tag == ShowCommandTag) command = static_cast<uint32_t>(ReadUnsigned(value, 4));
                else flags = static_cast<uint32_t>(ReadUnsigned(value, 4));
                break;
            case DeviceNameTag:
                if (length > 62 || length % 2 != 0) return DecodeResult::InvalidData;
                placement.DisplayDeviceName.clear();
                for (size_t i = 0; i < length; i += 2)
                {
                    const auto unit = static_cast<char16_t>(ReadUnsigned(value + i, 2));
                    if (unit == 0) return DecodeResult::InvalidData;
                    placement.DisplayDeviceName.push_back(unit);
                }
                break;
            case VirtualDesktopTag:
            {
                if (length != 16) return DecodeResult::InvalidData;
                GUID id{};
                id.Data1 = static_cast<uint32_t>(ReadUnsigned(value, 4));
                id.Data2 = static_cast<uint16_t>(ReadUnsigned(value + 4, 2));
                id.Data3 = static_cast<uint16_t>(ReadUnsigned(value + 6, 2));
                for (size_t i = 0; i < 8; ++i) id.Data4[i] = value[8 + i];
                placement.VirtualDesktopId = IsEmptyGuid(id) ? std::nullopt : std::optional<GUID>{id};
                break;
            }
            case SaveSequenceTag:
                if (length != 8) return DecodeResult::InvalidData;
                result.SaveSequence = ReadUnsigned(value, 8);
                break;
            }
            offset += length;
        }
        if (!hasNormal || !hasWorkArea || !hasDpi ||
            !ReadState(command, flags, result.Placement.PlacementState) || !IsValid(result.Placement))
        {
            return DecodeResult::InvalidData;
        }
        record = std::move(result);
        return DecodeResult::Success;
    }
}
