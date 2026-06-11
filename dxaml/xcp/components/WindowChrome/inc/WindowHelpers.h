// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "winuser.h"


class CUIElement;



namespace WindowHelpers
{

    // Helper functions for dealing with RECT struct
    namespace RectHelpers
    {
        inline LONG rectWidth(RECT const& rect) { return rect.right - rect.left; }
        inline LONG rectHeight(RECT const& rect) { return rect.bottom - rect.top; }
        inline LONG rectArea(RECT const& rect) { return rectWidth(rect) * rectHeight(rect); }
        inline bool rectIntersects(RECT const& rect1, RECT const& rect2)
        {
            RECT temp;
            return ::IntersectRect(&temp, &rect1, &rect2) ? true : false;
        }
        
        inline bool pointInRect(RECT const& rect, const wf::Point point)
        {
            const POINT pt{ static_cast<LONG>(std::round(point.X)), static_cast<LONG>(std::round(point.Y)) };
            return ::PtInRect(&rect, pt) ? true : false;
        }
    }

    SIZE GetPhysicalSize(HWND hwnd) noexcept;
    RECT GetClientWindowCoordinates(HWND hwnd) noexcept;
    RECT GetScreenWindowCoordinates(HWND hwnd) noexcept;
    float GetCurrentDpiScale(HWND hwnd) noexcept;
    wf::Size GetLogicalSize(const SIZE& physicalSize, float scale) noexcept;
    wf::Size GetLogicalSize(HWND hwnd) noexcept;
    wf::Rect GetLogicalWindowCoordinates(HWND hwnd) noexcept;
    wf::Point ClientPhysicaltoLogicalPoint(HWND targetWindow,  wf::Point physicalPt) noexcept;
    wf::Point ScreenPhysicaltoClientLogicalPoint(HWND targetWindow,  wf::Point physicalPt) noexcept;
    short ClampToShortMax(const long value, const short min) noexcept;

    RECT GetClientAreaLogicalRectForUIElement(_In_ CUIElement* uiElement) noexcept;
    RECT GetClientAreaPhysicalRectForUIElement(_In_ CUIElement* uiElement, float scale) noexcept;
    bool IsRTLLocale();
    void EnableRTL(HWND hwnd);



}