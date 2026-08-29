// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowChrome.g.h"

namespace DirectUI
{
    class DesktopWindowImpl;
    
    PARTIAL_CLASS(WindowChrome)
    {
    public:
        WindowChrome() = default;
        ~WindowChrome() = default;
        
        _Check_return_ HRESULT Initialize(_In_ HWND parent);

        _Check_return_ HRESULT SetTitleBar(_In_opt_ xaml::IUIElement* titleBar);
        _Check_return_ HRESULT ApplyStyling();
        void SetDesktopWindow(_In_ DesktopWindowImpl* window) {  m_desktopWindow = window; }

        bool HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, _Out_ LRESULT* pResult);
        void ResizeContainer(WPARAM wParam, LPARAM lParam);
        void MoveContainer(WPARAM wParam, LPARAM lParam);
        DesktopWindowImpl* GetDesktopWindowNoRef() const { return m_desktopWindow; }
        HWND GetPositioningBridgeWindowHandle() const;
        ctl::ComPtr<ixp::IAppWindow> GetAppWindow() const;
        bool CanDrag() const;
        void UpdateCanDragStatus(bool enabled);

        template <typename T>
        CUIElement* GetCUIElement(const TrackerPtr<T>& element) const
        {
            CUIElement* container = nullptr;
            if (element.Get())
            {
                ctl::ComPtr<UIElement> containerAsUE;
                element.As(&containerAsUE);
                container = static_cast<CUIElement*>(containerAsUE.Get()->GetHandle());
            }
            return container;
        }

        template <typename T>
        ctl::ComPtr<FrameworkElement> GetFrameworkElement(const TrackerPtr<T>& element) const
        {
            if (element.Get())
            {
                ctl::ComPtr<FrameworkElement> containerAsFE;
                containerAsFE = element.template AsOrNull<FrameworkElement>();
                return containerAsFE;
            }
            else
            {
                return nullptr;
            }
        }

        CUIElement* GetUserTitleBarNoRef() const
        {
            CUIElement* titleBar = nullptr;
            if(m_userTitleBar.Get())
            {
                titleBar = static_cast<UIElement*>(m_userTitleBar.Get())->GetHandle();
            }
            return titleBar;
        }
        
        ctl::ComPtr<ixp::IInputNonClientPointerSource> GetNonClientInputPtrSrc() const
        {
            return m_inputNonClientPtrSrc;
        }
        
        bool IsChromeActive() const;
        _Check_return_ HRESULT SetIsChromeActive(bool value);

        LRESULT SendMessageToDesktopWindow(UINT const message, WPARAM const wparam,  LPARAM const lparam);
        
    private:
        
        TrackerPtr<xaml::IUIElement> m_userTitleBar;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_titlebarSizeChangedEventHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_LoadedEventHandler;
        
        // ref to desktop window for calling its helper functions
        // instantiated when windowchrome object is created, set to null when DesktopWindow is closing
        DesktopWindowImpl* m_desktopWindow = nullptr;  
        ctl::ComPtr<ixp::IInputNonClientPointerSource> m_inputNonClientPtrSrc;

        // Called when the Content property changes.
        _Check_return_ IFACEMETHOD(OnContentChanged)(_In_ IInspectable* pOldContent, _In_ IInspectable* pNewContent) override;

    };
}