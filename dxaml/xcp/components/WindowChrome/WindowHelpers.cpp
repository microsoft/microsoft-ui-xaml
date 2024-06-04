// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowHelpers.h"
#include "uielement.h"
#include "contentroot.h"
#include "VisualTree.h"

SIZE WindowHelpers::GetPhysicalSize(HWND hwnd) noexcept
{
    RECT rect = {};
    ::GetClientRect(hwnd, &rect);
    const LONG windowsWidth = rect.right - rect.left;
    const LONG windowsHeight = rect.bottom - rect.top;
    return { windowsWidth, windowsHeight };
}

RECT WindowHelpers::GetClientWindowCoordinates(HWND hwnd) noexcept
{
    RECT rect = {};
    ::GetClientRect(hwnd, &rect);
    return rect;
}

RECT WindowHelpers::GetScreenWindowCoordinates(HWND hwnd) noexcept
{
    RECT rect = {};
    ::GetWindowRect(hwnd, &rect);
    return rect;
}

float WindowHelpers::GetCurrentDpiScale(HWND hwnd) noexcept
{
    const unsigned int dpi = ::GetDpiForWindow(hwnd);
    const float scale = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
    return scale;
}

//// Gets the logical (in DIPs) size of a physical size specified by the parameter physicalSize
//// Remarks:
//// XAML coordinate system is always in Display Independent Pixels (a.k.a DIPs or Logical). However Win32 GDI (because of legacy reasons)
//// in DPI mode "Per-Monitor and Per-Monitor (V2) DPI Awareness" is always in physical pixels.
//// The formula to transform is:
////     logical = (physical / dpi) + 0.5 // 0.5 is to ensure that we pixel snap correctly at the edges, this is necessary with odd DPIs like 1.25, 1.5, 1, .75
//// See also:
////   https://docs.microsoft.com/en-us/windows/desktop/LearnWin32/dpi-and-device-independent-pixels
////   https://docs.microsoft.com/en-us/windows/desktop/hidpi/high-dpi-desktop-application-development-on-windows#per-monitor-and-per-monitor-v2-dpi-awareness
wf::Size WindowHelpers::GetLogicalSize(const SIZE& physicalSize, float scale) noexcept
{
    // 0.5 is to ensure that we pixel snap correctly at the edges, this is necessary with odd DPIs like 1.25, 1.5, 1, .75
    const float logicalWidth = (physicalSize.cx / scale) + 0.5f;
    const float logicalHeight = (physicalSize.cy / scale) + 0.5f;
    return {logicalWidth, logicalHeight};
}

wf::Size WindowHelpers::GetLogicalSize(HWND hwnd) noexcept
{
    const float scale = GetCurrentDpiScale(hwnd);
    return GetLogicalSize(GetPhysicalSize(hwnd), scale);
}

wf::Rect WindowHelpers::GetLogicalWindowCoordinates(HWND hwnd) noexcept
{
    RECT rect = GetClientWindowCoordinates(hwnd);
    const float scale = GetCurrentDpiScale(hwnd);
    wf::Size windowSize = GetLogicalSize(hwnd);
    return 
    {
        rect.left / scale,    // X
        rect.top / scale,     // Y
        windowSize.Width,     // Width
        windowSize.Height     // Height
    };
}

wf::Point WindowHelpers::ClientPhysicaltoLogicalPoint(HWND targetWindow,  wf::Point physicalPt) noexcept
{
    wf::Point logicalPt{};
    POINT physicalPoint { static_cast<LONG>(physicalPt.X), static_cast<LONG>(physicalPt.Y)};
    const float scale = GetCurrentDpiScale(targetWindow);
    logicalPt = { physicalPoint.x / scale, physicalPoint.y / scale };
    return logicalPt;
}


wf::Point WindowHelpers::ScreenPhysicaltoClientLogicalPoint(HWND targetWindow,  wf::Point physicalPt) noexcept
{
    wf::Point logicalPt{};
    POINT physicalPoint { static_cast<LONG>(physicalPt.X), static_cast<LONG>(physicalPt.Y)};
    
    // in case of ScreenToClient failing, the returned value will be (0,0)
    if (::ScreenToClient(targetWindow, &physicalPoint))
    {
        const float scale = GetCurrentDpiScale(targetWindow);
        logicalPt = { physicalPoint.x / scale, physicalPoint.y / scale };
    }
    else
    {
        ASSERT(L"Physical point to logical point conversion failed");
    }
    return logicalPt;
}

short WindowHelpers::ClampToShortMax(const long value, const short min) noexcept
{
    return static_cast<short>(std::clamp(value,
        static_cast<long>(min),
        static_cast<long>(SHRT_MAX)));
}

// gets client logical rect for a given ui element
RECT WindowHelpers::GetClientAreaLogicalRectForUIElement(_In_ CUIElement* uiElement) noexcept
{
    const XRECTF logicalRect = {
        0.0f,
        0.0f,
        uiElement->GetActualWidth(),
        uiElement->GetActualHeight()
    };

    if (logicalRect.Width > 0 && logicalRect.Height > 0)
    {
        xref_ptr<CGeneralTransform> transform;
        IFCFAILFAST(uiElement->TransformToVisual(nullptr, &transform));
        XRECTF clientRect = {};
        IFCFAILFAST(transform->TransformRect(logicalRect, &clientRect));

        return  {
            static_cast<LONG>(std::round(clientRect.X)),
            static_cast<LONG>(std::round(clientRect.Y)),
            static_cast<LONG>(std::round(clientRect.Width + clientRect.X)),
            static_cast<LONG>(std::round(clientRect.Height + clientRect.Y))
        };

    }

    return {};
}

// gets client physical rect for a given ui element
RECT WindowHelpers::GetClientAreaPhysicalRectForUIElement(_In_ CUIElement* uiElement, float scale) noexcept
{
    RECT clientLogicalRect = GetClientAreaLogicalRectForUIElement(uiElement);
    if (clientLogicalRect.left != clientLogicalRect.right &&
             clientLogicalRect.top != clientLogicalRect.bottom)
    {
        return  {
            static_cast<LONG>(clientLogicalRect.left * scale),
            static_cast<LONG>(clientLogicalRect.top * scale),
            static_cast<LONG>(clientLogicalRect.right * scale),
            static_cast<LONG>(clientLogicalRect.bottom * scale)
        };
    }
    return {};
}

// if Right to Left language is used in UI thread
// gets set later in lifetime of Dxaml core so may not be correct during init
bool WindowHelpers::IsRTLLocale()
{
    LCID lcid = MAKELCID(::GetThreadUILanguage(), SORT_DEFAULT);
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    DWORD readingLayout = 0;
    ::LCIDToLocaleName(lcid, localeName, LOCALE_NAME_MAX_LENGTH, 0);
    ::GetLocaleInfoEx (localeName,
                       LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
                       reinterpret_cast<wchar_t*>(&readingLayout),
                       sizeof(readingLayout) / sizeof(wchar_t));

    // Value of 1 indicates RTL language. See MSDN for LOCALE_IREADINGLAYOUT.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd373806(v=vs.85).aspx
    return readingLayout == 1;
}

void WindowHelpers::EnableRTL(HWND hwnd)
{
    LONG_PTR exStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ::SetWindowLongPtr(hwnd, GWL_EXSTYLE, (exStyle | WS_EX_LAYOUTRTL));
}