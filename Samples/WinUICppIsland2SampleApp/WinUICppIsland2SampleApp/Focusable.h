// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "winrt/Microsoft.UI.Input.h"
#include "winrt/Microsoft.UI.Content.h"

class IFocusable
{
public:
    virtual HWND GetHwnd() const = 0;
    virtual winrt::Microsoft::UI::Input::InputFocusNavigationHost GetFocusNavigationHost() const = 0;
    virtual winrt::Microsoft::UI::Input::FocusNavigationResult NavigateFocus(winrt::Microsoft::UI::Input::FocusNavigationRequest) = 0;
};

class HwndFocusable : public IFocusable
{
public:
    HwndFocusable(HWND hwnd);
    HWND GetHwnd() const override;
    winrt::Microsoft::UI::Input::InputFocusNavigationHost GetFocusNavigationHost() const override;
    winrt::Microsoft::UI::Input::FocusNavigationResult NavigateFocus(winrt::Microsoft::UI::Input::FocusNavigationRequest) override;

private:
    HWND m_hwnd;
};

class ContentFocusable : public IFocusable
{
public:
    ContentFocusable(winrt::Microsoft::UI::Content::IContentSiteBridge bridge);
    HWND GetHwnd() const override;
    winrt::Microsoft::UI::Input::InputFocusNavigationHost GetFocusNavigationHost() const override;
    winrt::Microsoft::UI::Input::FocusNavigationResult NavigateFocus(winrt::Microsoft::UI::Input::FocusNavigationRequest request) override;

private:
    winrt::Microsoft::UI::Input::InputFocusNavigationHost m_focusNavigationHost{ nullptr };
};