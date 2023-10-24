// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class ContentManager
    {
    public:
        ContentManager(_In_ DependencyObject* pOwner, bool isUwpWindowContent = false);
        ~ContentManager();

        xaml_controls::IScrollViewer* GetRootScrollViewer();
        xaml_controls::IScrollContentPresenter* GetRootSVContentPresenter();
        xaml::IUIElement* GetContent();
        _Check_return_ HRESULT SetContent(_In_opt_ xaml::IUIElement* pContent);
        _Check_return_ HRESULT OnWindowSizeChanged();

        _Check_return_ HRESULT ClearRootScrollViewer();

    private:
        // The owner is either a DirectUI::Window object, or a DirectUI::XamlIsland
        DependencyObject* m_owner;

        // This is "true" only in UWP mode when m_owner is a DirectUI::Window
        // it is "false" when serving a XamlIsland, including Desktop.
        bool m_isUwpWindowContent;

        TrackerPtr<xaml::IUIElement> m_Content;

        TrackerPtr<xaml_controls::IScrollViewer> m_RootScrollViewer;
        TrackerPtr<xaml_controls::IScrollContentPresenter> m_RootSVContentPresenter;
        EventRegistrationToken m_tokRootScrollViewerSizeChanged;

        _Check_return_ HRESULT CreateRootScrollViewer(_In_ xaml::IUIElement* pContent);
        static _Check_return_ HRESULT OnRootScrollViewerSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);
    };
}
