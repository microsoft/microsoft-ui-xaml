// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBar.g.h"

namespace DirectUI
{
    class ButtonBase;
    class Page;

    // Represents the AppBar control
    PARTIAL_CLASS(AppBar)
    {
    public:
        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* returnValue) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* returnValue) override;

        void SetMode(AppBarMode mode) { m_Mode = mode; }
        AppBarMode GetMode() { return m_Mode; }

        _Check_return_ HRESULT SetOwner(_In_opt_ Page* pOwner);
        _Check_return_ HRESULT GetOwner(_Outptr_result_maybenull_ Page** ppPage);

        void SetOnLoadFocusState(_In_ xaml::FocusState focusState) { m_onLoadFocusState = focusState; }

        virtual _Check_return_ HRESULT ContainsElement(_In_ DependencyObject* pElement, _Out_ bool *pContainsElement);
        bool IsExpandButton(_In_ UIElement* element);

        virtual _Check_return_ HRESULT OnOpeningImpl(_In_ IInspectable* pArgs);
        virtual _Check_return_ HRESULT OnOpenedImpl(_In_ IInspectable* pArgs);
        virtual _Check_return_ HRESULT OnClosingImpl(_In_ IInspectable* pArgs);
        virtual _Check_return_ HRESULT OnClosedImpl(_In_ IInspectable* pArgs);

        _Check_return_ HRESULT ProcessTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            const bool isBackward,
            const bool didCycleFocusAtRootVisualScope,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsTabStopOverridden
            ) override;

        _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* pReturnValue);

    protected:
        AppBar();
        ~AppBar() override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT OnVisibilityChanged() override;

        _Check_return_ HRESULT ChangeVisualState(bool useTransitions) override;

        IFACEMETHOD(OnPointerPressed)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnRightTapped)(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        virtual _Check_return_ HRESULT OnContentRootSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);

        virtual _Check_return_ HRESULT RestoreSavedFocusImpl(_In_opt_ DependencyObject* savedFocusedElement, xaml::FocusState savedFocusState);

        virtual _Check_return_ HRESULT UpdateTemplateSettings();

        virtual _Check_return_ HRESULT GetShouldOpenUp(_Out_ bool* shouldOpenUp);

        virtual _Check_return_ HRESULT HasRightLabelDynamicPrimaryCommand(_Out_ bool* hasRightLabelDynamicPrimaryCommand);
        virtual _Check_return_ HRESULT HasNonLabeledDynamicPrimaryCommand(_Out_ bool* hasNonLabeledDynamicPrimaryCommand);

        _Check_return_ HRESULT HasSpaceForAppBarToOpenDown(_Out_ bool* hasSpace);

        double GetCompactHeight() const { return m_compactHeight; }
        double GetMinimalHeight() const { return m_minimalHeight; }
        double GetContentHeight() const { return m_contentHeight; }

        // Dismisses an inline appbar if it is not sticky.
        _Check_return_ HRESULT TryDismissInlineAppBar();
        _Check_return_ HRESULT TryDismissInlineAppBar(_Out_ bool* isAppBarDismissed);

        // Template parts.
        TrackerPtr<xaml_controls::IGrid>            m_tpLayoutRoot;
        TrackerPtr<xaml::IFrameworkElement>         m_tpContentRoot;
        TrackerPtr<xaml_primitives::IButtonBase>    m_tpExpandButton;
        TrackerPtr<xaml::IVisualStateGroup>         m_tpDisplayModesStateGroup;

        bool m_openedWithExpandButton;

    private:
        _Check_return_ HRESULT OnIsOpenChanged(bool isOpen);
        _Check_return_ HRESULT OnIsOpenChangedForAutomation(_In_ const PropertyChangedParams& args);

        _Check_return_ HRESULT OnIsStickyChanged();

        _Check_return_ HRESULT OnLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnLayoutUpdated(_In_ IInspectable* sender, _In_ IInspectable* args);
        _Check_return_ HRESULT OnSizeChanged(_In_ IInspectable* sender, _In_ xaml::ISizeChangedEventArgs* args);
        _Check_return_ HRESULT OnXamlRootChanged(_In_ xaml::IXamlRoot* pSender, _In_ xaml::IXamlRootChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnExpandButtonClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
        _Check_return_ HRESULT OnDisplayModesStateChanged(_In_ IInspectable* pSender, xaml::IVisualStateChangedEventArgs* pArgs);

        _Check_return_ HRESULT SetFocusOnAppBar();
        _Check_return_ HRESULT RestoreSavedFocus();

        _Check_return_ HRESULT RefreshContentHeight(_Out_opt_ bool *didChange);

        _Check_return_ HRESULT SetExpandButtonAutomationName(_In_opt_ ButtonBase* expandButton, bool isAppBarExpanded);
        _Check_return_ HRESULT SetExpandButtonToolTip(_In_opt_ ButtonBase* expandButton, bool isAppBarExpanded);

        _Check_return_ HRESULT SetupOverlayState();
        _Check_return_ HRESULT TeardownOverlayState();

        _Check_return_ HRESULT CreateLTEs();
        _Check_return_ HRESULT PositionLTEs();
        _Check_return_ HRESULT DestroyLTEs();

        _Check_return_ HRESULT OnOverlayElementPointerPressed(IInspectable* pSender, xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT TryQueryDisplayModesStatesGroup();

        bool ShouldUseParentedLTE();

        _Check_return_ HRESULT ReevaluateIsOverlayVisible();
        _Check_return_ HRESULT UpdateOverlayElementBrush();
        _Check_return_ HRESULT UpdateTargetForOverlayAnimations();
        _Check_return_ HRESULT PlayOverlayOpeningAnimation();
        _Check_return_ HRESULT PlayOverlayClosingAnimation();

        double GetMinCompactHeight() const
        {
            return m_minCompactHeight;
        }

        void SetMinCompactHeight(double value)
        {
            if (m_minCompactHeight != value)
            {
                m_minCompactHeight = value;
            }
        }

        void SetCompactHeight(double value) 
        {
            if (m_compactHeight != value)
            {
                m_compactHeight = value;
            }
        }

        void SetMinimalHeight(double value)
        {
            if (m_minimalHeight != value)
            {
                m_minimalHeight = value;
            }
        }

        void SetContentHeight(double value) 
        {
            if (m_contentHeight != value)
            {
                m_contentHeight = value;
            }
        }

        AppBarMode m_Mode;

        // Focus state to be applied on loaded.
        xaml::FocusState m_onLoadFocusState;

        // Owner, if this AppBar is owned by a Page using TopAppBar/BottomAppBar.
        ctl::WeakRefPtr m_wpOwner;

        ctl::EventPtr<FrameworkElementLoadedEventCallback>                  m_loadedEventHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback>                m_unloadedEventHandler;
        ctl::EventPtr<FrameworkElementLayoutUpdatedEventCallback>           m_layoutUpdatedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback>             m_sizeChangedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback>             m_contentRootSizeChangedEventHandler;
        ctl::EventPtr<XamlRootChangedEventCallback>                         m_xamlRootChangedEventHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback>                         m_expandButtonClickEventHandler;
        ctl::EventPtr<VisualStateGroupCurrentStateChangedEventCallback>     m_displayModeStateChangedEventHandler;

        xref_ptr<CUIElement> m_layoutTransitionElement;
        xref_ptr<CUIElement> m_overlayLayoutTransitionElement;

        TrackerPtr<xaml::IUIElement>                        m_parentElementForLTEs;
        TrackerPtr<xaml::IFrameworkElement>                 m_overlayElement;
        ctl::EventPtr<UIElementPointerPressedEventCallback> m_overlayElementPointerPressedEventHandler;

        ctl::WeakRefPtr m_savedFocusedElementWeakRef;
        xaml::FocusState m_savedFocusState;

        bool m_isInOverlayState;
        bool m_isChangingOpenedState;
        bool m_hasUpdatedTemplateSettings;
        bool m_hasExpandButtonCustomAutomationName;

        // We refresh this value in the OnSizeChanged() & OnContentSizeChanged() handlers.
        double m_contentHeight;

        double m_minCompactHeight;
        double m_compactHeight;
        double m_minimalHeight;

        bool m_isOverlayVisible;
        TrackerPtr<xaml_animation::IStoryboard> m_overlayOpeningStoryboard;
        TrackerPtr<xaml_animation::IStoryboard> m_overlayClosingStoryboard;

    }; // class AppBar
}
