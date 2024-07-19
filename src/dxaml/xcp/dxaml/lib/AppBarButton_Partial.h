// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarButton.g.h"
#include "CascadingMenuHelper.h"

namespace DirectUI
{
    // Represents the AppBarButton control
    PARTIAL_CLASS(AppBarButton)
    {
        friend class AppBarButtonHelpers;
        friend class AppBarButtonAutomationPeer;
        
    public:
        _Check_return_ HRESULT SetOverflowStyleParams(_In_ bool hasIcons, _In_ bool hasToggleButtons, _In_ bool hasKeyboardAcceleratorText);

        // ICommandBarLabeledElement implementation
        _Check_return_ HRESULT SetDefaultLabelPositionImpl(_In_ xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition);
        _Check_return_ HRESULT GetHasBottomLabelImpl(_Out_ BOOLEAN* hasBottomLabel);
        _Check_return_ HRESULT GetHasRightLabelImpl(_Out_ BOOLEAN * hasRightLabel);

        // ICommanBarElement2 implementation
        _Check_return_ HRESULT get_IsInOverflowImpl(_Out_ BOOLEAN* pValue);
        
        _Check_return_ HRESULT get_KeyboardAcceleratorTextOverrideImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_KeyboardAcceleratorTextOverrideImpl(_In_opt_ HSTRING value);

        void SetInputMode(_In_ DirectUI::InputDeviceType inputDeviceTypeUsedToOpenOverflow)
        {
            m_inputDeviceTypeUsedToOpenOverflow = inputDeviceTypeUsedToOpenOverflow;
        };
        
        // ISubMenuOwner implementation
        _Check_return_ HRESULT get_IsSubMenuOpenImpl(_Out_ BOOLEAN* pValue);

        _Check_return_ HRESULT get_IsSubMenuPositionedAbsolutelyImpl(_Out_ BOOLEAN* pValue)
        {
            *pValue = FALSE;
            return S_OK;
        }

        _Check_return_ HRESULT get_ParentOwnerImpl(_Outptr_ xaml_controls::ISubMenuOwner** ppValue);
        _Check_return_ HRESULT put_ParentOwnerImpl(_In_ xaml_controls::ISubMenuOwner* pValue);

        _Check_return_ HRESULT SetSubMenuDirectionImpl(BOOLEAN isSubMenuDirectionUp);
        _Check_return_ HRESULT PrepareSubMenuImpl();
        _Check_return_ HRESULT OpenSubMenuImpl(wf::Point position);
        _Check_return_ HRESULT PositionSubMenuImpl(wf::Point position);
        _Check_return_ HRESULT ClosePeerSubMenusImpl();
        _Check_return_ HRESULT CloseSubMenuImpl();
        _Check_return_ HRESULT CloseSubMenuTreeImpl();
        _Check_return_ HRESULT DelayCloseSubMenuImpl();
        _Check_return_ HRESULT CancelCloseSubMenuImpl();
        _Check_return_ HRESULT RaiseAutomationPeerExpandCollapseImpl(_In_ BOOLEAN isOpen);

    protected:
        AppBarButton();

        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* args) override;

        // Updates our visual state when the value of our IsCompact property changes
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions = true) override;

        _Check_return_ HRESULT OnClick() override;

        _Check_return_ HRESULT OnVisibilityChanged() override;
        
        _Check_return_ HRESULT OnCommandChanged(_In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue) override;

        // If we have a flyout that represents a sub-menu, then we want to open it to the right
        // of the app bar button if it's in the overflow.  We'll override OpenAssociatedFlyout
        // to ensure that we handle it correctly.
        _Check_return_ HRESULT OpenAssociatedFlyout() override;

    private:
        _Check_return_ HRESULT GetHasLabelAtPosition(_In_ xaml_controls::CommandBarDefaultLabelPosition labelPosition, _Out_ BOOLEAN* hasLabelAtPosition);
        _Check_return_ HRESULT GetEffectiveLabelPosition(_Out_ xaml_controls::CommandBarDefaultLabelPosition* effectiveLabelPosition);
        _Check_return_ HRESULT UpdateInternalStyles();

        _Check_return_ HRESULT CreateStoryboardForWidthAdjustmentsForLabelOnRightStyle(
            _Out_ xaml_animation::IStoryboard** storyboard);

        _Check_return_ HRESULT StartAnimationForWidthAdjustments();
        _Check_return_ HRESULT StopAnimationForWidthAdjustments();

        // LabelOnRightStyle doesn't work in AppBarButton/AppBarToggleButton Reveal Style.
        // Animate the width to NaN if width is not overrided and right-aligned labels and no LabelOnRightStyle. 
        TrackerPtr<xaml_animation::IStoryboard> m_widthAdjustmentsForLabelOnRightStyleStoryboard;

        bool m_isWithToggleButtons;
        bool m_isWithIcons;
        xaml_controls::CommandBarDefaultLabelPosition m_defaultLabelPosition;
        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpenOverflow;

        TrackerPtr<xaml_controls::ITextBlock> m_tpKeyboardAcceleratorTextLabel;

        // We won't actually set the label-on-right style unless we've applied the template,
        // because we won't have the label-on-right style from the template until we do.
        bool m_isTemplateApplied;
        
        // We need to adjust our visual state to account for CommandBarElements that have keyboard accelerator text.
        bool m_isWithKeyboardAcceleratorText = false;
        double m_maxKeyboardAcceleratorTextWidth = 0;

        // If we have a keyboard accelerator attached to us and the app has not set a tool tip on us,
        // then we'll create our own tool tip.  We'll use this flag to indicate that we can unset or
        // overwrite that tool tip as needed if the keyboard accelerator is removed or the button
        // moves into the overflow section of the app bar or command bar.
        bool m_ownsToolTip;
        
        // Helper to which to delegate cascading menu functionality.
        TrackerPtr<CascadingMenuHelper> m_menuHelper;

        // Helpers to track the current opened state of the flyout.
        bool m_isFlyoutClosing{ false };
        ctl::EventPtr<FlyoutBaseOpenedEventCallback> m_flyoutOpenedHandler;
        ctl::EventPtr<FlyoutBaseClosedEventCallback> m_flyoutClosedHandler;

        // Holds the last position that its flyout was opened at.
        // Used to reposition the flyout on size changed.
        wf::Point m_lastFlyoutPosition;
    };
}
