// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIAHostEnvironmentInfo.h"
#include <XamlOneCoreTransforms.h>
#include <DComp.h>
#include <XamlIslandRoot.h>
#include <DXamlServices.h>
#include <XcpMath.h>
#include <RootScale.h>

UIAHostEnvironmentInfo::UIAHostEnvironmentInfo() { }

UIAHostEnvironmentInfo::UIAHostEnvironmentInfo(_In_ HWND elementWindow, _In_ HWND transformWindow)
    : m_elementWindow(elementWindow)
    , m_transformWindow(transformWindow)
{ }

UIAHostEnvironmentInfo::UIAHostEnvironmentInfo(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    m_weakXamlIslandRoot = xref::weakref_ptr<CXamlIslandRoot>(xamlIslandRoot);
}

UIAHostEnvironmentInfo::~UIAHostEnvironmentInfo()
{ }

UIAHostEnvironmentInfo::UIAHostEnvironmentInfo(const UIAHostEnvironmentInfo& other)
{ *this = other; }

const UIAHostEnvironmentInfo& UIAHostEnvironmentInfo::operator=(_In_ const UIAHostEnvironmentInfo& rhs)
{
    m_elementWindow = rhs.m_elementWindow;
    m_transformWindow = rhs.m_transformWindow;
    m_weakXamlIslandRoot = rhs.m_weakXamlIslandRoot;
    return *this;
}

_Check_return_ xref_ptr<CXamlIslandRoot> UIAHostEnvironmentInfo::GetXamlIslandRoot()
{
    return (m_weakXamlIslandRoot.lock());
}

bool UIAHostEnvironmentInfo::GlobalSpaceToUiaSpace(_Inout_ POINT* point)
{
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In this scenario we need to convert from RasterizedClient to Logical coordinates,
        // as we're consuming the result of AutomationPeer::GetBoundingRectangleCore, which
        // explicitly returns RasterizedClient coordinates.
        // See comments in CUIAWrapper::GetVisualRelativeBoundingRectangle.
        const auto xamlIsland = GetXamlIslandRoot();
        const auto scale = RootScale::GetRasterizationScaleForElementWithFallback(xamlIsland.get());
        const auto physicalPoint = ConvertPOINTToXPOINTF(*point);
        const auto logicalPoint = physicalPoint / scale;
        *point = ConvertXPOINTFToPOINT(logicalPoint);
        return true;
    }

    if (m_transformWindow)
    {
        XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ClientToScreen call
        return !!::ClientToScreen(m_transformWindow, point);
    }

    auto islandRoot = m_weakXamlIslandRoot.lock();
    if (islandRoot != nullptr)
    {
        islandRoot->ClientPhysicalToScreenPhysical(*point);
        return true;
    }
    else
    {
        return false;
    }
}

bool UIAHostEnvironmentInfo::UiaSpaceToGlobalSpace(_Inout_ POINT* point) const
{
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In this scenario there's no need to convert, as when we're going in this direction,
        // we take a visual-relative coordinate and run it straight down the XAML tree, which is
        // natively in visual relative coordinates.  This may seem confusing as GlobalSpaceToUiaSpace
        // does a conversion from RasterizedClient to Logical.  In that case we are using the results
        // of AutomationPeer::GetBoundingRectangleCore, which explicitly returns RasterizedClient coordinates.
        // See comments in CUIAWrapper::GetVisualRelativeBoundingRectangle.
        return true;
    }

    if (m_transformWindow)
    {
        XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ScreenToClient call
        return !!::ScreenToClient(m_transformWindow, point);
    }

    auto islandRoot = m_weakXamlIslandRoot.lock();
    if (islandRoot != nullptr)
    {
        islandRoot->ScreenPhysicalToClientPhysical(*point);
        return true;
    }
    else
    {
        return false;
    }
}

bool UIAHostEnvironmentInfo::GlobalSpaceToUiaSpace(_Inout_ XRECTF* rect)
{
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In this scenario we need to convert from RasterizedClient to Logical coordinates,
        // as we're consuming the result of AutomationPeer::GetBoundingRectangleCore, which
        // explicitly returns RasterizedClient coordinates.
        // See comments in CUIAWrapper::GetVisualRelativeBoundingRectangle.
        const auto xamlIsland = GetXamlIslandRoot();
        const auto scale = RootScale::GetRasterizationScaleForElementWithFallback(xamlIsland.get());
        *rect = ((*rect) / scale);
        return true;
    }

    POINT ptTopLeft = {static_cast<LONG>(rect->X), static_cast<LONG>(rect->Y)};
    bool result = GlobalSpaceToUiaSpace(&ptTopLeft);
    rect->X = static_cast<XFLOAT>(ptTopLeft.x);
    rect->Y = static_cast<XFLOAT>(ptTopLeft.y);
    return result;
}

HWND UIAHostEnvironmentInfo::GetElementWindow() const
{
    FAIL_FAST_ASSERT(IsHwndBased());
    return m_elementWindow;
}

void UIAHostEnvironmentInfo::SetElementWindow(HWND hwnd)
{
    FAIL_FAST_ASSERT(IsHwndBased());
    m_elementWindow = hwnd;
}

HWND UIAHostEnvironmentInfo::GetTransformWindow() const
{
    FAIL_FAST_ASSERT(IsHwndBased());
    return m_transformWindow;
}

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
GUID UIAHostEnvironmentInfo::GetXamlIslandEndpointId()
{
    auto islandRoot = m_weakXamlIslandRoot.lock();
    return islandRoot->GetCompositionIslandId();
}
#endif
