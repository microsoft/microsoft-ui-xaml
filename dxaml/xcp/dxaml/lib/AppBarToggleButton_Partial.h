// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarToggleButton.g.h"

namespace DirectUI
{
    // Represents the AppBarToggleButton control
    PARTIAL_CLASS(AppBarToggleButton)
    {
        friend class AppBarButtonHelpers;
        
    public:
        AppBarToggleButton();
        
        _Check_return_ HRESULT SetOverflowStyleParams(_In_ bool hasIcons, _In_ bool hasKeyboardAcceleratorText);

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

    protected:
        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* args) override;

        // Updates our visual state when the value of our IsCompact property changes
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions = true) override;

        _Check_return_ HRESULT OnClick() override;

        _Check_return_ HRESULT OnVisibilityChanged() override;
        
        _Check_return_ HRESULT OnCommandChanged(_In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue) override;
        
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

        xaml_controls::CommandBarDefaultLabelPosition m_defaultLabelPosition;
        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpenOverflow;

        TrackerPtr<xaml_controls::ITextBlock> m_tpKeyboardAcceleratorTextLabel;

        // We won't actually set the label-on-right style unless we've applied the template,
        // because we won't have the label-on-right style from the template until we do.
        bool m_isTemplateApplied;

       
        // We need to adjust our visual state to account for CommandBarElements that use Icons.
        bool m_isWithIcons;
        
        // We need to adjust our visual state to account for CommandBarElements that have keyboard accelerator text.
        bool m_isWithKeyboardAcceleratorText = false;
        double m_maxKeyboardAcceleratorTextWidth = 0;

        // If we have a keyboard accelerator attached to us and the app has not set a tool tip on us,
        // then we'll create our own tool tip.  We'll use this flag to indicate that we can unset or
        // overwrite that tool tip as needed if the keyboard accelerator is removed or the button
        // moves into the overflow section of the app bar or command bar.
        bool m_ownsToolTip;
    };
}
