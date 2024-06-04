// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitView.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SplitView)
    {
    protected:
        ~SplitView() override;

        _Check_return_ HRESULT PrepareState() override;

        IFACEMETHOD(OnApplyTemplate)() override;

        // Invoked when the KeyDown event is raised.
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) final;

        _Check_return_ HRESULT ProcessTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            const bool isBackward,
            const bool didCycleFocusAtRootVisualScope,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsTabStopOverridden
            ) override;

        _Check_return_ HRESULT ProcessCandidateTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_ DependencyObject* pCandidateTabStopElement,
            _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
            const bool isBackward,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsCandidateTabStopOverridden
            ) override;

        _Check_return_ HRESULT GetFirstFocusableElementOverride(
            _Outptr_result_maybenull_ DependencyObject** firstFocusable) override;

        _Check_return_ HRESULT GetLastFocusableElementOverride(
            _Outptr_ DependencyObject** lastFocusable) override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions) override;

    public:
        _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* pReturnValue);

    private:
        _Check_return_ HRESULT OnIsPaneOpenChanged(bool isOpen);
        _Check_return_ HRESULT OnDisplayModeChanged();

        _Check_return_ HRESULT OnLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnXamlRootChanged(_In_ xaml::IXamlRoot* pSender, _In_ xaml::IXamlRootChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnOuterDismissElementPointerPressed(IInspectable* pSender, xaml_input::IPointerRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnDisplayModeStateChanged(_In_ IInspectable* sender, _In_ xaml::IVisualStateChangedEventArgs* args);

        _Check_return_ HRESULT ProcessTabStopInternal(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            bool isForward,
            _Outptr_result_maybenull_ DependencyObject** ppNewTabStop
            );

        _Check_return_ HRESULT SetupOuterDismissLayer() noexcept;
        _Check_return_ HRESULT TeardownOuterDismissLayer();

        _Check_return_ HRESULT RegisterForDisplayModeStatesChangedEvent();
        _Check_return_ HRESULT OnPaneOpenedOrClosed(bool isPaneOpen);

        static _Check_return_ HRESULT CreatePolygonalPath(
            _In_ xaml_controls::IGrid* hostElement,
            size_t numPoints,
            _Outptr_ xaml_shapes::IPath** ppPath
            );

        static _Check_return_ HRESULT UpdatePolygonalPath(
            _In_ xaml_shapes::IPath* path,
            size_t numPoints,
            _In_ wf::Point* points
            );

        ctl::EventPtr<FrameworkElementLoadedEventCallback>      m_loadedEventHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback>    m_unloadedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_sizeChangedEventHandler;
        ctl::EventPtr<XamlRootChangedEventCallback>             m_xamlRootChangedEventHandler;

        ctl::EventPtr<VisualStateGroupCurrentStateChangedEventCallback> m_displayModeStateChangedEventHandler;

        // These elements are used to form the outer dismiss layer that detects user interactions
        // outside of the SplitView bounds.  There are 4 polygonal elements arranged around the SplitView.
        // They are split up into individual elements rather than being a single path element because of
        // limitations in DComps input handling which would not correctly pass input through an element
        // on top of another DComp element (such as a WebView).
        //
        // We defer creation of these elements when the SplitView takes up the full window,
        // which is the common case.  Some apps, such as Spartan, have the SplitView nested.
        ctl::ComPtr<xaml_primitives::IPopup>    m_outerDismissLayerPopup;
        ctl::ComPtr<xaml_controls::IGrid>                 m_dismissHostElement;
        ctl::ComPtr<xaml_shapes::IPath>                   m_topDismissElement;
        ctl::ComPtr<xaml_shapes::IPath>                   m_bottomDismissElement;
        ctl::ComPtr<xaml_shapes::IPath>                   m_leftDismissElement;
        ctl::ComPtr<xaml_shapes::IPath>                   m_rightDismissElement;

        ctl::EventPtr<UIElementPointerPressedEventCallback>     m_dismissLayerPointerPressedEventHandler;

        // Template Parts
        TrackerPtr<xaml::IFrameworkElement> m_tpPaneRoot;
        TrackerPtr<xaml::IFrameworkElement> m_tpContentRoot;
        TrackerPtr<xaml::IVisualStateGroup> m_tpDisplayModeStates;

        bool m_isPaneOpeningOrClosing{ false };
    };
}
