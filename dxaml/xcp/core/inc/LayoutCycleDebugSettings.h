// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

class LayoutCycleDebugSettings
{
public:
    static DirectUI::LayoutCycleDebugBreakLevel GetDebugBreakLevel() { return m_layoutCycleDebugBreaks; }
    static void SetDebugBreakLevel(DirectUI::LayoutCycleDebugBreakLevel level) { m_layoutCycleDebugBreaks = level; }

    static DirectUI::LayoutCycleTracingLevel GetTracingLevel() { return m_layoutCycleTracing; }
    static void SetTracingLevel(DirectUI::LayoutCycleTracingLevel level) { m_layoutCycleTracing = level; }

    static bool ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel level)
    {
        return (m_layoutCycleDebugBreaks >= level) && IsDebuggerPresent();
    }

    static bool ShouldTrace(DirectUI::LayoutCycleTracingLevel level)
    {
        return (m_layoutCycleTracing >= level);
    }

private:
    static DirectUI::LayoutCycleDebugBreakLevel m_layoutCycleDebugBreaks;
    static DirectUI::LayoutCycleTracingLevel m_layoutCycleTracing;
};

DirectUI::LayoutCycleDebugBreakLevel LayoutCycleDebugSettings::m_layoutCycleDebugBreaks{ DirectUI::LayoutCycleDebugBreakLevel::Off };
DirectUI::LayoutCycleTracingLevel LayoutCycleDebugSettings::m_layoutCycleTracing{ DirectUI::LayoutCycleTracingLevel::Off };

