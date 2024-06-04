// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutPresenter.g.h"
#include "MenuFlyoutKeyPressProcess.h"
#include "FlyoutBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(MenuFlyoutPresenter)
    {
        // Give MenuFlyout friend access so it can call private method SetOwnerMenuFlyout().
        friend class MenuFlyout;

        // Give MenuFlyoutItem friend access so it can call private methods GetOwnerMenuFlyout(),
        // HandleUpOrDownKey(), and GetContainsCheckedItems().
        friend class MenuFlyoutItemBase;
        friend class MenuFlyoutItem;
        friend class MenuFlyoutSubItem;
        friend class ToggleMenuFlyoutItem;

        //Give access to the private method HandleUpOrDownKey
        friend class KeyPress::MenuFlyoutPresenter;
        friend class KeyPress::MenuFlyout;

    private:
        // Can be negative. (-1) means nothing focused.
        INT m_iFocusedIndex;

        // Weak reference to the menu that ultimately owns this MenuFlyoutPresenter.
        ctl::WeakRefPtr m_wrOwningMenu;

        // Weak reference to the parent MenuFlyout.
        ctl::WeakRefPtr m_wrParentMenuFlyout;

        // Weak reference to the owner of this menu.
        // Only populated if this is the presenter for an ISubMenuOwner.
        ctl::WeakRefPtr m_wrOwner;

        // Weak reference to the sub-presenter that was created by a child menu owner.
        ctl::WeakRefPtr m_wrSubPresenter;

        // Whether ItemsSource contains at least one ToggleMenuFlyoutItem.
        bool m_containsToggleItems;
        
        // Whether ItemsSource contains at least one MenuFlyoutItem with an Icon.
        bool m_containsIconItems;

        // Whether ItemsSource contains at least one MenuFlyoutItem or ToggleMenuFlyoutItem with KeyboardAcceleratorText.
        bool m_containsItemsWithKeyboardAcceleratorText;

        bool m_animationInProgress;

        bool m_isSubPresenter;

        UINT m_depth{ 0 };

        FlyoutBase::MajorPlacementMode m_mostRecentPlacement;

        // References the panels in the template.
        TrackerPtr<xaml::IUIElement> m_tpOuterBorder;

        TrackerPtr<xaml_animation::ITimeline> m_tpTopPortraitTimeline;
        TrackerPtr<xaml_animation::ITimeline> m_tpBottomPortraitTimeline;
        TrackerPtr<xaml_animation::ITimeline> m_tpLeftLandscapeTimeline;
        TrackerPtr<xaml_animation::ITimeline> m_tpRightLandscapeTimeline;

        TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;

        // DEAD_CODE_REMOVAL: These are no longer used
        ctl::EventPtr<TimelineCompletedEventCallback> m_epTopPortraitCompletedHandler;
        ctl::EventPtr<TimelineCompletedEventCallback> m_epBottomPortraitCompletedHandler;
        ctl::EventPtr<TimelineCompletedEventCallback> m_epLeftLandscapeCompletedHandler;
        ctl::EventPtr<TimelineCompletedEventCallback> m_epRightLandscapeCompletedHandler;

    public:
        _Check_return_ HRESULT GetOwnerName(_Out_ HSTRING* pName);

        _Check_return_ HRESULT DelayCloseMenuFlyoutSubItem();

        _Check_return_ HRESULT CancelCloseMenuFlyoutSubItem();

        _Check_return_ HRESULT UpdateTemplateSettings();

        bool IsSubPresenter()
        {
            return m_isSubPresenter;
        }

        static _Check_return_ HRESULT GetParentMenuFlyoutSubItem(
            _In_ CDependencyObject* nativeDO,
            _Outptr_ CDependencyObject** ppMenuFlyoutSubItem);

        static _Check_return_ HRESULT GetPositionInSetHelper(const ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase>& item, _Out_ INT* returnValue);
        static _Check_return_ HRESULT GetSizeOfSetHelper(const ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase>& item, _Out_ INT* returnValue);

        // IMenuPresenter implementation
        _Check_return_ HRESULT get_OwnerImpl(_Outptr_result_maybenull_ xaml_controls::ISubMenuOwner** ppValue);
        _Check_return_ HRESULT put_OwnerImpl(_In_opt_ xaml_controls::ISubMenuOwner* pValue);
        _Check_return_ HRESULT get_OwningMenuImpl(_Outptr_result_maybenull_ xaml_controls::IMenu** ppValue);
        _Check_return_ HRESULT put_OwningMenuImpl(_In_opt_ xaml_controls::IMenu* pValue);
        _Check_return_ HRESULT get_SubPresenterImpl(_Outptr_result_maybenull_ xaml_controls::IMenuPresenter** ppValue);
        _Check_return_ HRESULT put_SubPresenterImpl(_In_opt_ xaml_controls::IMenuPresenter* pValue);

        _Check_return_ HRESULT CloseSubMenuImpl();

        void SetDepth(_In_ UINT depth);
        UINT GetDepth();

    protected:
        // Initializes a new instance.
        MenuFlyoutPresenter();

        _Check_return_ HRESULT PrepareState() override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        // Responds to the KeyDown event.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // Prepares the specified element to display the specified item.
        IFACEMETHOD(PrepareContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        // Undoes the effects of the PrepareContainerForItemOverride method.
        IFACEMETHOD(ClearContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        // Called when the ItemsSource property changes.
        _Check_return_ HRESULT OnItemsSourceChanged(
            _In_ IInspectable* pNewValue) override;

        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            bool bUseTransitions) override;

        // Get MenuFlyoutPresenter template parts and create the sources if they are not already there.
        IFACEMETHOD(OnApplyTemplate)() override;

        // PointerExited event handler.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerEntered event handler.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

    private:
        _Check_return_ HRESULT OnLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT SetInitialFocus();

        _Check_return_ HRESULT HandleUpOrDownKey(
            BOOLEAN isDownKey);

        _Check_return_ HRESULT HandleKeyDownLeftOrEscape();

        // Get the parent MenuFlyout.
        _Check_return_ HRESULT GetParentMenuFlyout(
            _Outptr_result_maybenull_ MenuFlyout** ppParentMenuFlyout);

        // Sets the parent MenuFlyout.
        _Check_return_ HRESULT SetParentMenuFlyout(
            _In_ MenuFlyout* pParentMenuFlyout);

        _Check_return_ HRESULT UpdateVisualStateForPlacement(
            FlyoutBase::MajorPlacementMode placement);

        _Check_return_ HRESULT ResetVisualState();

        _Check_return_ HRESULT AttachEntranceAnimationCompleted(
            _In_reads_(nStateNameLength + 1) _Null_terminated_ const WCHAR* pszStateName,
            XUINT32 nStateNameLength,
            _Outptr_result_maybenull_ xaml_animation::ITimeline** ppTimeline,
            _In_ ctl::EventPtr<TimelineCompletedEventCallback>* pCompletedEvent);

        _Check_return_ HRESULT DetachEntranceAnimationCompletedHandlers();

        _Check_return_ HRESULT OnEntranceAnimationCompleted(
            _In_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        // Returns true if the ItemsSource contains at least one ToggleMenuFlyoutItem; false otherwise.
        bool GetContainsToggleItems() const
        {
            return m_containsToggleItems;
        }

        // Returns true if the ItemsSource contains at least one MenuFlyoutItem with an Icon; false otherwise.
        bool GetContainsIconItems() const
        {
            return m_containsIconItems;
        }

        // Returns true if the ItemsSource contains at least one MenuFlyoutItem with an Icon; false otherwise.
        bool GetContainsItemsWithKeyboardAcceleratorText() const
        {
            return m_containsItemsWithKeyboardAcceleratorText;
        }

        // Cycles the focus through focusable items in this presenter
        _Check_return_ HRESULT CycleFocus(
            BOOLEAN shouldCycleDown,
            xaml::FocusState focusState);

        // Ensure the initial focused index to validate m_iFocusedIndex in case of
        // overriding the focus item during the Opened event.
        _Check_return_ HRESULT EnsureInitialFocusIndex();

        bool GetPointerPosition(wf::Point clientLogicalPointerPosition,
            _Out_ XPOINTF& pointerPosition);

        _Check_return_ HRESULT GetSubPresenterBounds(_In_ xaml::IUIElement* pSubPresenterAsUIE, 
                                                     _Out_ XRECTF_RB *pSubPresenterBounds);
    };
}
