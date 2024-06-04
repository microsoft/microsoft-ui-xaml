// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GroupItem.g.h"

namespace DirectUI
{
    // forward declarations
    class Selector;

    /// <summary>
    /// Represents a UI for group of items.
    /// </summary>
    PARTIAL_CLASS(GroupItem)
    {
        friend class ItemsControl;
        friend class ItemContainerGenerator;

    public:

        // Provides the behavior for the Arrange pass of layout.  Classes
        // can override this method to define their own Arrange pass
        // behavior.    
        IFACEMETHOD(ArrangeOverride)(
            // The computed size that is used to arrange the content.
            _In_ wf::Size arrangeSize,
            // The size of the control.
            _Out_ wf::Size* returnValue) override;

        // Invoked whenever application code or internal processes call
        // ApplyTemplate.
        IFACEMETHOD(OnApplyTemplate)() override;
        
        // Call ChangeSelectorItemsVisualState on our ItemsControl template part.
        // This results in a call to ChangeVisualState on all child SelectorItems (including items inside another GroupItem),
        // with optimizations for the virtualization provided by IOrientedVirtualizingPanel.
        virtual _Check_return_ HRESULT ChangeSelectorItemsVisualState(_In_ bool bUseTransitions);

        // determines if this element should be transitioned using the passed in transition
        IFACEMETHOD(GetCurrentTransitionContext)(
            _In_ INT layoutTickId, 
            _Out_ ThemeTransitionContext* pReturnValue) 
            override;

        // determines if mutations are going fast
        IFACEMETHOD(IsCollectionMutatingFast)(
            _Out_ BOOLEAN* returnValue)
            override;

        IFACEMETHOD(GetDropOffsetToRoot)(
            _Out_ wf::Point* pReturnValue)
            override;
           
        // Internal methods 
        void SetCollectionViewGroup(_In_ xaml_data::ICollectionViewGroup *pGroup);
        _Check_return_ HRESULT GetCollectionViewGroup(_Outptr_ xaml_data::ICollectionViewGroup **ppGroup);
        _Check_return_ HRESULT GetTemplatedItemsControl(_Outptr_ xaml_controls::IItemsControl **ppItemsControl);
        
        // Places focus on the group header, if possible. Note the default GroupItem style has a non-focusable HeaderContent
        // part, meaning that there needs to be focusable content within HeaderContent for this to work.
        _Check_return_ HRESULT FocusHeader(_In_ xaml::FocusState focusState, _Out_ BOOLEAN* pDidSetFocus);
        
        // Obtains the index of the group this GroupItem is representing.
        _Check_return_ HRESULT GetGroupIndex(_Out_ INT* pGroupIndex);

        // Unloads all containers to main generator container recycling queue
        _Check_return_ HRESULT Recycle();

        _Check_return_ HRESULT GetHeaderContent(_Outptr_ IControl** ppHeaderContent);

        void SetGenerator(_In_opt_ xaml_controls::IItemContainerGenerator* const pGenerator);
    
    protected:
        // Initializes a new instance of the GroupItem class.
        GroupItem();
        ~GroupItem() override;

        _Check_return_ HRESULT PrepareItemContainer(_In_ IInspectable* pItem, ItemsControl* pItemsControl);
        _Check_return_ HRESULT ClearContainerForItem(_In_ IInspectable* pItem);
 
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
        
        // Called when the GroupItem receives focus.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs)
            override;

        // Called when the GroupItem loses focus.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs)
            override;
            
    private:
        TrackerPtr<xaml_controls::IItemContainerGenerator> m_tpGenerator;
        TrackerPtr<xaml_controls::IItemsControl> m_tpItemsControl;
        ctl::WeakRefPtr m_wrParentListViewBaseWeakRef;
        TrackerPtr<xaml_data::ICollectionViewGroup> m_tpCVG;
        EventRegistrationToken m_itemsChangedToken{};
        EventRegistrationToken m_headerKeyDownToken{};
        xaml::Visibility m_previousVisibility;
        TrackerPtr<IControl> m_tpHeaderControl;
        BOOLEAN m_bWasHidden;

        _Check_return_ HRESULT Hide();
        _Check_return_ HRESULT OnItemsChangedHandler(
            _In_ IInspectable* sender, 
            _In_ xaml_primitives::IItemsChangedEventArgs* args);
            
        // Update our parent ListView when group focus changes so it can manage
        // the currently focused header, if appropriate.
        _Check_return_ HRESULT FocusChanged(
            _In_ BOOLEAN hasFocus,
            _In_ xaml::FocusState howFocusChanged);
            
        // Handler for KeyDown on our HeaderContent part.
        _Check_return_ HRESULT OnHeaderKeyDown(
            _In_opt_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);
        
        // Helper to update m_tpHeaderControl. Attaches and/or detaches OnHeaderKeyDown.
        _Check_return_ HRESULT SetHeaderContentReference(_In_opt_ IControl* pHeaderContent);
    };
}
