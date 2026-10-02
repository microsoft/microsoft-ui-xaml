// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementAdapter.h"

#include <limits>
#include <algorithm>
#include <utility>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        constexpr int64_t MinCoordinate = (std::numeric_limits<int32_t>::min)();
        constexpr int64_t MaxCoordinate = (std::numeric_limits<int32_t>::max)();

        struct FlagMapping
        {
            bool NativeFlags::* Member;
            PlacementFlags EngineFlag;
        };

        constexpr FlagMapping FlagMappings[] =
        {
            { &NativeFlags::NoActivate, PlacementFlags::NoActivate },
            { &NativeFlags::KeepHidden, PlacementFlags::KeepHidden },
            { &NativeFlags::Arranged, PlacementFlags::Arranged },
            { &NativeFlags::RestoreToArranged, PlacementFlags::RestoreToArranged },
            { &NativeFlags::RestoreToMaximized, PlacementFlags::RestoreToMaximized },
            { &NativeFlags::AllowSizing, PlacementFlags::AllowSizing },
            { &NativeFlags::UseVirtualDesktopId, PlacementFlags::VirtualDesktopId },
        };

        bool IsValidRequest(const NativeRequest& request)
        {
            Snapshot snapshot{};
            snapshot.NormalRect = request.NormalRect;
            snapshot.WorkArea = request.WorkArea;
            snapshot.Dpi = request.Dpi;
            snapshot.SnapRect = request.ArrangeRect;
            snapshot.DisplayDeviceName = request.DeviceName;
            if (!IsValid(snapshot)) return false;

            const auto& flags = request.Flags;
            if (flags.UseVirtualDesktopId != request.VirtualDesktopId.has_value() ||
                (flags.Arranged || flags.RestoreToArranged) != request.ArrangeRect.has_value() ||
                (flags.RestoreToMaximized && flags.RestoreToArranged))
            {
                return false;
            }

            if (flags.KeepHidden &&
                (!flags.NoActivate || request.ShowCommand != NativeShowCommand::NoChange))
            {
                return false;
            }

            switch (request.ShowCommand)
            {
            case NativeShowCommand::Normal:
                return !flags.RestoreToMaximized && !flags.RestoreToArranged;
            case NativeShowCommand::Maximize:
                return !flags.Arranged && !flags.RestoreToMaximized && !flags.RestoreToArranged;
            case NativeShowCommand::Minimize:
                return !flags.Arranged;
            case NativeShowCommand::NoChange:
                return flags.KeepHidden && !flags.Arranged &&
                    !flags.RestoreToMaximized && !flags.RestoreToArranged;
            default:
                return false;
            }
        }

        RECT ToEngineRect(const Rect& rect) noexcept
        {
            // IsValidRequest has checked positive dimensions and representable edges.
            return { rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height };
        }

        bool TryToEngineRect(const Rect& rect, RECT& result) noexcept
        {
            const int64_t right = int64_t{rect.X} + rect.Width;
            const int64_t bottom = int64_t{rect.Y} + rect.Height;
            if (rect.Width <= 0 || rect.Height <= 0 ||
                rect.X < MinCoordinate || rect.Y < MinCoordinate ||
                right > MaxCoordinate || bottom > MaxCoordinate)
            {
                return false;
            }
            result = { rect.X, rect.Y, static_cast<LONG>(right), static_cast<LONG>(bottom) };
            return true;
        }

        bool FromEngineRect(const RECT& source, Rect& result) noexcept
        {
            const int64_t width = int64_t{source.right} - source.left;
            const int64_t height = int64_t{source.bottom} - source.top;
            if (width <= 0 || width > MaxCoordinate || height <= 0 || height > MaxCoordinate)
            {
                return false;
            }
            result = { source.left, source.top, static_cast<int32_t>(width), static_cast<int32_t>(height) };
            return true;
        }

        bool IsUsableMonitor(const MonitorDescription& monitor) noexcept
        {
            return monitor.Dpi > 0 &&
                monitor.WorkArea.Width > 0 &&
                monitor.WorkArea.Height > 0;
        }

        // The engine uses LONG subtraction/addition and MulDiv. Reject a conservative
        // envelope that cannot fit those intermediates before calling it. This is a
        // representability check, not another placement/fit algorithm.
        bool CanAdjustAxis(LONG start, LONG end, LONG oldStart, LONG oldEnd,
            LONG newStart, LONG newEnd, UINT oldDpi, UINT newDpi) noexcept
        {
            const int64_t oldSize = int64_t{oldEnd} - oldStart;
            const int64_t newSize = int64_t{newEnd} - newStart;
            const int64_t size = int64_t{end} - start;
            if (oldSize <= 0 || newSize <= 0 || size <= 0 || !oldDpi || !newDpi) return false;
            const auto magnitude = [](int64_t value) { return value < 0 ? -value : value; };
            const auto offset = (std::max)(magnitude(int64_t{start} - oldStart),
                magnitude(int64_t{oldEnd} - end));
            if (offset > MaxCoordinate || oldSize > MaxCoordinate || newSize > MaxCoordinate) return false;
            const int64_t scaledOffset = (offset * newSize + oldSize - 1) / oldSize;
            const int64_t scaledSize = (size * newDpi + oldDpi - 1) / oldDpi;
            if (scaledOffset > MaxCoordinate || scaledSize > MaxCoordinate) return false;
            const auto origin = (std::max)(magnitude(newStart), magnitude(newEnd));
            // Also covers the two OffsetRect operations in KeepRectOnMonitor.
            return origin + newSize + 2 * scaledOffset + 2 * scaledSize <= MaxCoordinate;
        }

        int64_t IntersectionArea(const RECT& left, const RECT& right) noexcept
        {
            const auto width = (std::min)(left.right, right.right) -
                (std::max)(left.left, right.left);
            const auto height = (std::min)(left.bottom, right.bottom) -
                (std::max)(left.top, right.top);
            return width > 0 && height > 0 ? int64_t{width} * height : 0;
        }

        int64_t DistanceToRect(const RECT& source, const RECT& target) noexcept
        {
            const auto x = source.left < target.left ? target.left - source.left :
                source.left > target.right ? source.left - target.right : 0;
            const auto y = source.top < target.top ? target.top - source.top :
                source.top > target.bottom ? source.top - target.bottom : 0;
            return int64_t{x} * x + int64_t{y} * y;
        }
    }

    bool TryCreateEnginePlacement(const NativeRequest& request, PlacementEx& placement)
    {
        if (!IsValidRequest(request)) return false;

        PlacementEx result{};
        result.normalRect = ToEngineRect(request.NormalRect);
        result.workArea = ToEngineRect(request.WorkArea);
        result.dpi = static_cast<UINT>(request.Dpi);
        std::copy(request.DeviceName.begin(), request.DeviceName.end(), result.deviceName);
        if (request.ArrangeRect) result.arrangeRect = ToEngineRect(*request.ArrangeRect);
        if (request.VirtualDesktopId) result.virtualDesktopId = *request.VirtualDesktopId;

        switch (request.ShowCommand)
        {
        case NativeShowCommand::Normal: result.showCmd = SW_NORMAL; break;
        case NativeShowCommand::Maximize: result.showCmd = SW_MAXIMIZE; break;
        case NativeShowCommand::Minimize: result.showCmd = SW_MINIMIZE; break;
        // SW_HIDE is the engine representation, not a promise that SetPlacement can
        // preserve hidden state on every backend. Application must gate that separately.
        case NativeShowCommand::NoChange: result.showCmd = SW_HIDE; break;
        }
        for (const auto& mapping : FlagMappings)
        {
            if (request.Flags.*(mapping.Member)) result.flags |= mapping.EngineFlag;
        }
        if (!result.IsValid()) return false;

        placement = result;
        return true;
    }

    bool TryReadEnginePlacement(const PlacementEx& placement, NativeRequest& request)
    {
        NativeRequest result{};
        if (placement.dpi > MaxCoordinate ||
            !FromEngineRect(placement.normalRect, result.NormalRect) ||
            !FromEngineRect(placement.workArea, result.WorkArea))
        {
            return false;
        }
        result.Dpi = static_cast<int32_t>(placement.dpi);
        const auto nameEnd = std::find(std::begin(placement.deviceName), std::end(placement.deviceName), L'\0');
        if (nameEnd == std::end(placement.deviceName)) return false;
        result.DeviceName.assign(std::begin(placement.deviceName), nameEnd);

        PlacementFlags supportedFlags = PlacementFlags::None;
        for (const auto& mapping : FlagMappings)
        {
            result.Flags.*(mapping.Member) = placement.HasFlag(mapping.EngineFlag);
            supportedFlags |= mapping.EngineFlag;
        }
        if ((placement.flags & ~supportedFlags) != PlacementFlags::None) return false;

        if (result.Flags.Arranged || result.Flags.RestoreToArranged)
        {
            Rect arranged{};
            if (!FromEngineRect(placement.arrangeRect, arranged)) return false;
            result.ArrangeRect = arranged;
        }
        if (result.Flags.UseVirtualDesktopId) result.VirtualDesktopId = placement.virtualDesktopId;

        switch (placement.showCmd)
        {
        case SW_NORMAL: result.ShowCommand = NativeShowCommand::Normal; break;
        case SW_MAXIMIZE: result.ShowCommand = NativeShowCommand::Maximize; break;
        case SW_MINIMIZE: result.ShowCommand = NativeShowCommand::Minimize; break;
        case SW_HIDE: result.ShowCommand = NativeShowCommand::NoChange; break;
        default: return false;
        }
        if (!IsValidRequest(result)) return false;

        request = std::move(result);
        return true;
    }

    bool TryCreateEnginePlacement(
        const Snapshot& snapshot,
        bool allowSizing,
        PlacementEx& placement)
    {
        NativeRequest request{};
        request.NormalRect = snapshot.NormalRect;
        request.WorkArea = snapshot.WorkArea;
        request.Dpi = snapshot.Dpi;
        request.DeviceName = snapshot.DisplayDeviceName;
        request.Flags.AllowSizing = allowSizing;
        if (snapshot.SnapRect.has_value())
        {
            request.ArrangeRect = snapshot.SnapRect;
            request.Flags.Arranged = true;
        }
        return TryCreateEnginePlacement(request, placement);
    }

    bool TrySelectEngineMonitor(
        const Snapshot& snapshot,
        const Topology& topology,
        MonitorDescription& monitor)
    {
        if (topology.Monitors.empty()) return false;

        RECT source{};
        if (!TryToEngineRect(snapshot.NormalRect, source)) return false;
        const MonitorDescription* best = nullptr;
        for (const auto& candidate : topology.Monitors)
        {
            if (IsUsableMonitor(candidate) && candidate.DeviceName == snapshot.DisplayDeviceName)
            {
                monitor = candidate;
                return true;
            }
        }

        int64_t bestArea = 0;
        for (const auto& candidate : topology.Monitors)
        {
            if (!IsUsableMonitor(candidate)) continue;
            RECT workArea{};
            if (!TryToEngineRect(candidate.WorkArea, workArea)) continue;
            const auto area = IntersectionArea(source, workArea);
            if (area > bestArea)
            {
                bestArea = area;
                best = &candidate;
            }
        }

        if (best == nullptr)
        {
            int64_t bestDistance = 0;
            for (const auto& candidate : topology.Monitors)
            {
                if (!IsUsableMonitor(candidate)) continue;
                RECT workArea{};
                if (!TryToEngineRect(candidate.WorkArea, workArea)) continue;
                const auto distance = DistanceToRect(source, workArea);
                if (best == nullptr || distance < bestDistance)
                {
                    best = &candidate;
                    bestDistance = distance;
                }
            }
        }

        if (best == nullptr) return false;
        monitor = *best;
        return true;
    }

    bool TryMoveEngineToMonitor(
        PlacementEx& placement,
        const MonitorDescription& monitor)
    {
        if (!IsUsableMonitor(monitor)) return false;
        MonitorData target{};
        if (!TryToEngineRect(monitor.WorkArea, target.workArea)) return false;
        target.dpi = static_cast<UINT>(monitor.Dpi);
        auto canAdjust = [&](const RECT& rect)
        {
            return CanAdjustAxis(rect.left, rect.right, placement.workArea.left, placement.workArea.right,
                target.workArea.left, target.workArea.right, placement.dpi, target.dpi) &&
                CanAdjustAxis(rect.top, rect.bottom, placement.workArea.top, placement.workArea.bottom,
                    target.workArea.top, target.workArea.bottom, placement.dpi, target.dpi);
        };
        if (!canAdjust(placement.normalRect) ||
            ((placement.HasFlag(PlacementFlags::Arranged) || placement.HasFlag(PlacementFlags::RestoreToArranged)) &&
                !canAdjust(placement.arrangeRect))) return false;
        return SUCCEEDED(StringCchCopy(
            target.deviceName, ARRAYSIZE(target.deviceName),
            reinterpret_cast<const wchar_t*>(monitor.DeviceName.c_str()))) &&
            (placement.MoveToMonitor(target), true);
    }

    bool TryCascadeEngine(PlacementEx& placement, int32_t offset)
    {
        if (offset <= 0) return false;
        if (int64_t{placement.normalRect.left} + offset < MinCoordinate ||
            int64_t{placement.normalRect.top} + offset < MinCoordinate ||
            int64_t{placement.normalRect.right} + offset < MinCoordinate ||
            int64_t{placement.normalRect.bottom} + offset < MinCoordinate ||
            int64_t{placement.normalRect.left} + offset > MaxCoordinate ||
            int64_t{placement.normalRect.top} + offset > MaxCoordinate ||
            int64_t{placement.normalRect.right} + offset > MaxCoordinate ||
            int64_t{placement.normalRect.bottom} + offset > MaxCoordinate)
        {
            return false;
        }
        placement.Cascade(offset);
        return true;
    }

}
