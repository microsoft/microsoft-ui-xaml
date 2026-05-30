// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PivotHeaderManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    class PivotHeaderItem :
        public PivotHeaderItemGenerated
    {

    public:
        PivotHeaderItem();

        _Check_return_ HRESULT UpdateVisualState(_In_ bool useTransitions);
        _Check_return_ HRESULT SetHeaderManagerCallbacks(_In_ xaml_controls::IPivotHeaderManagerItemEvents* pHeaderManager);
        _Check_return_ HRESULT StoreDesiredSize(_Out_ BOOLEAN* hasChanged);

        // For the Locked <-> Normal state change we want to hide the LTE because
        // it will translate onto screen (it's usually off screen) which isn't the
        // desired behavior. We have to wait untl the state change animations are
        // complete to replace it so we need the visual state group completion callback.
        // Because we don't need to listen for every header item's completed event
        // we optimize and only subscribe to the first item's event.
        _Check_return_ HRESULT SetSubscribeToStateChangeCallback(_In_ bool shouldSubscribe);

        void ClearIsHovered();
        void SetIsSelected(_In_ bool isSelected);
        void SetShouldShowFocusWhenSelected(_In_ bool shouldShowFocusState);

        // IControlOverrides methods
        _Check_return_ HRESULT OnPointerCaptureLostImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerEnteredImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerExitedImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerPressedImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerReleasedImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnTappedImpl(_In_ xaml_input::ITappedRoutedEventArgs *e) override;

        // IFrameworkElementOverrides methods
        _Check_return_ HRESULT OnApplyTemplateImpl() override;

    protected:
        _Check_return_ HRESULT InitializeImpl(_In_opt_ IInspectable* pOuter) override;

    private:

        _Check_return_ HRESULT SubscribeToVisualStateGroupCompleted();
        _Check_return_ HRESULT UnsubscribeFromVisualStateGroupCompleted();

        _Check_return_ HRESULT EnsureSelectionStateGroup();

        _Check_return_ HRESULT FindVisualStateGroup(_In_ HSTRING hName, _Outptr_result_maybenull_ xaml::IVisualStateGroup** ppStateGroup);

        _Check_return_ HRESULT OnVisualStateChanged(_In_ IInspectable* pSender, _In_ xaml::IVisualStateChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnTemplatePropertyChanged(xaml::IDependencyObject* /*sender*/, xaml::IDependencyProperty* /*dp*/);

        Private::TrackerPtr<xaml::IFrameworkElement> m_tpRootGrid;
        Private::TrackerPtr<xaml::IUIElement> m_tpSelectedPipe;
        Private::TrackerPtr<xaml::IVisualStateGroup> m_tpSelectionStateGroup;

        xaml_controls::IPivotHeaderManagerItemEvents* m_pManagerNoRef;
        wf::Size m_lastDesiredSize;
        bool m_shouldSubscribeToStateChange;
        bool m_isSelected;
        bool m_isPressed;
        bool m_isPointerOver;
        bool m_shouldShowFocusStateWhenSelected;
        bool m_selectionStateGroupCached;
        EventRegistrationToken m_visualStateChangedToken;

        static const WCHAR s_disabledState[];
        static const WCHAR s_selectedState[];
        static const WCHAR s_selectedPointerOverState[];
        static const WCHAR s_selectedPressedState[];
        static const WCHAR s_unselectedState[];
        static const WCHAR s_unselectedPointerOverState[];
        static const WCHAR s_unselectedPressedState[];
        static const WCHAR s_unselectedLockedState[];
        static const WCHAR s_selectionStateGroup[];
        static const WCHAR s_focusedState[];
        static const WCHAR s_unfocusedState[];
    };

    ActivatableClass(PivotHeaderItem);
} } } } } XAML_ABI_NAMESPACE_END

