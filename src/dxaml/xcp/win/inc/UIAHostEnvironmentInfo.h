// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlIslandRoot.h>

// UIAHostEnvironmentInfo is an abstration to help CUIAWindow and CUIAWrapper run correctly
// whether XAML is parented by an HWND or by an island.
class UIAHostEnvironmentInfo
{
public:
    UIAHostEnvironmentInfo();
    UIAHostEnvironmentInfo(_In_ HWND elementWindow, _In_ HWND transformWindow);
    UIAHostEnvironmentInfo(_In_ CXamlIslandRoot* xamlIslandRoot);
    ~UIAHostEnvironmentInfo();

    UIAHostEnvironmentInfo(const UIAHostEnvironmentInfo& other);
    const UIAHostEnvironmentInfo& operator=(_In_ const UIAHostEnvironmentInfo& rhs);

    // Coordinate spaces are temporarily variable due to OneCoreTransforms mode:
    // Global space: the coordinate space returned by GetGlobalBounds and TransformToRoot
    //  - For a "normal" non-OneCoreTransforms XAML app, global space is "client physical" space
    //  - For a OneCoreTransforms app, this is "client logical", or visual-relative to the CoreWindow
    // UIA space: the coordinate space UIA communicates in
    //  - For a "normal" non-OneCoreTransforms XAML app, global space is "screen physical" space
    //  - For a OneCoreTransforms app, this is "client logical", or visual-relative to the CoreWindow
    // http://osgvsowi/11110898 -- rationalize these, it's getting too confusing.
    bool GlobalSpaceToUiaSpace(_Inout_ POINT* point);
    bool UiaSpaceToGlobalSpace(_Inout_ POINT* point) const;
    bool GlobalSpaceToUiaSpace(_Inout_ XRECTF* rect);

    HWND GetElementWindow() const;
    void SetElementWindow(HWND hwnd);

    HWND GetTransformWindow() const;

    GUID GetXamlIslandEndpointId();

    _Check_return_ xref_ptr<CXamlIslandRoot> GetXamlIslandRoot();
    bool IsHwndBased() const { return m_elementWindow != nullptr || m_transformWindow != nullptr; }

private:
    // Window containing XAML element. Can be different from m_transformWindow
    // if the element is in a windowed popup.
    HWND m_elementWindow = nullptr;
    // Window used for screen/client transforms. This is the Jupiter window
    // because element layout is relative to the Jupiter window.
    HWND m_transformWindow = nullptr;

    xref::weakref_ptr<CXamlIslandRoot> m_weakXamlIslandRoot;
};
