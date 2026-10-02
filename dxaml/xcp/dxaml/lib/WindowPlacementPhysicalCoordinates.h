// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>

namespace DirectUI::WindowPlacementPersistence
{
    // Placement snapshots and the engine both use physical screen pixels.
    class PhysicalCoordinateScope
    {
    public:
        PhysicalCoordinateScope() noexcept
        {
            m_previous = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            if (!m_previous)
            {
                m_previous = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
            }
        }
        ~PhysicalCoordinateScope() noexcept
        {
            if (m_previous) ::SetThreadDpiAwarenessContext(m_previous);
        }
        PhysicalCoordinateScope(const PhysicalCoordinateScope&) = delete;
        PhysicalCoordinateScope& operator=(const PhysicalCoordinateScope&) = delete;
        bool IsValid() const noexcept { return m_previous != nullptr; }

    private:
        DPI_AWARENESS_CONTEXT m_previous{};
    };
}
