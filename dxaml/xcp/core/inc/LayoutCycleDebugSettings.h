// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

class LayoutCycleDebugSettings
{
public:
    static DirectUI::LayoutCycleDebugBreakLevel GetDebugBreakLevel() { return m_layoutCycleDebugBreakLevel; }
    static void SetDebugBreakLevel(DirectUI::LayoutCycleDebugBreakLevel level) { m_layoutCycleDebugBreakLevel = level; }

    static DirectUI::LayoutCycleTracingLevel GetTracingLevel() { return m_layoutCycleTracingLevel; }
    static void SetTracingLevel(DirectUI::LayoutCycleTracingLevel level) { m_layoutCycleTracingLevel = level; }

    static bool ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel level)
    {
        return (m_layoutCycleDebugBreakLevel >= level) && IsDebuggerPresent();
    }

    static bool ShouldTrace(DirectUI::LayoutCycleTracingLevel level)
    {
        return (m_layoutCycleTracingLevel >= level);
    }

private:
    static DirectUI::LayoutCycleDebugBreakLevel m_layoutCycleDebugBreakLevel;
    static DirectUI::LayoutCycleTracingLevel m_layoutCycleTracingLevel;
};

__declspec(selectany)
DirectUI::LayoutCycleDebugBreakLevel LayoutCycleDebugSettings::m_layoutCycleDebugBreakLevel{ DirectUI::LayoutCycleDebugBreakLevel::None };

__declspec(selectany)
DirectUI::LayoutCycleTracingLevel LayoutCycleDebugSettings::m_layoutCycleTracingLevel{ DirectUI::LayoutCycleTracingLevel::None };
