// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <string>
#include <guiddef.h>

// The parsed, in-memory form of a persisted window placement. This is the WinUI-owned view of a
// placement, independent of PlacementEx types. Production code translates between this struct and
// PlacementEx at the native boundary; the serializer converts between this struct and the WPL1
// wire format.

namespace WindowPlacement
{
    // A rectangle in screen coordinates. Kept as plain int32 so the format and serializer layers
    // do not depend on windows.h. Matches a Win32 RECT field-for-field.
    struct WindowPlacementRect
    {
        int32_t left = 0;
        int32_t top = 0;
        int32_t right = 0;
        int32_t bottom = 0;

        bool operator==(const WindowPlacementRect& other) const
        {
            return left == other.left && top == other.top &&
                   right == other.right && bottom == other.bottom;
        }
        bool operator!=(const WindowPlacementRect& other) const { return !(*this == other); }
    };

    // The set of placement fields v1 can persist. Each optional field carries an explicit presence
    // flag so a reader can distinguish "absent" from "zero". TAG_NORMAL_RECT is the only required
    // field; a placement with hasNormalRect == false is not a usable saved placement.
    struct WindowPlacementData
    {
        WindowPlacementRect normalRect;         // TAG_NORMAL_RECT (required)
        bool hasNormalRect = false;

        WindowPlacementRect workArea;           // TAG_WORK_AREA
        bool hasWorkArea = false;

        WindowPlacementRect arrangeRect;        // TAG_ARRANGE_RECT
        bool hasArrangeRect = false;

        uint32_t dpi = 0;                       // TAG_DPI
        bool hasDpi = false;

        uint32_t showCmd = 0;                   // TAG_SHOW_CMD (Win32 SW_*)
        bool hasShowCmd = false;

        uint32_t flags = 0;                     // TAG_FLAGS (WindowPlacementFlags, known bits only)
        bool hasFlags = false;

        std::wstring deviceName;                // TAG_DEVICE_NAME (no trailing NUL)
        bool hasDeviceName = false;

        GUID virtualDesktopId = {};             // TAG_VIRTUAL_DESKTOP_ID
        bool hasVirtualDesktopId = false;
    };
}
