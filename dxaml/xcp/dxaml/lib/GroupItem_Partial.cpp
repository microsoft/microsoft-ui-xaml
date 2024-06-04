// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GroupItem.g.h"
#include "GroupItemAutomationPeer.g.h"
#include "ListViewBase.g.h"
#include "ItemContainerGenerator.g.h"
#include "GroupStyle.g.h"
#include "Panel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

GroupItem::GroupItem()
    : m_bWasHidden(FALSE)
{
    memset(&m_itemsChangedToken, 0, sizeof(EventRegistrationToken));
    m_previousVisibility = xaml::Visibility_Collapsed;
}

GroupItem::~GroupItem()
{
    auto spGenerator = m_tpGenerator.GetSafeReference();
    if (spGenerator &&
        m_itemsChangedToken.value != 0)
    {
        IGNOREHR(spGenerator->remove_ItemsChanged(m_itemsChangedToken));
    }

    auto spHeaderControl = m_tpHeaderControl.GetSafeReference();
    if (spHeaderControl)
    {
        IGNOREHR(spHeaderControl.Cast<Control>()->remove_KeyDown(m_headerKeyDownToken));
    }
}

// Handler for KeyDown on our HeaderContent part.
_Check_return_
HRESULT
GroupItem::OnHeaderKeyDown(
    _In_opt_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    ctl::ComPtr<IListViewBase> spIParentListViewBase;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isHandled = FALSE;

    if (!m_tpHeaderControl)
    {
        return S_OK;
    }

    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    IFC_RETURN(m_tpHeaderControl.Cast<Control>()->get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        return S_OK;
    }

    IFC_RETURN(m_wrParentListViewBaseWeakRef.As(&spIParentListViewBase));
    if (spIParentListViewBase)
    {
        ListViewBase* pParentListViewBase = spIParentListViewBase.Cast<ListViewBase>();
        wsy::VirtualKey originalKey = wsy::VirtualKey_None;
        wsy::VirtualKey key = wsy::VirtualKey_None;

        IFC_RETURN(pArgs->get_OriginalKey(&originalKey));
        IFC_RETURN(pArgs->get_Key(&key));

        IFC_RETURN(pParentListViewBase->OnGroupHeaderKeyDown(ctl::as_iinspectable(this), originalKey, key, &isHandled));

        if (isHandled)
        {
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }

    return S_OK;
}

// Helper to get the HeaderContent corresponding to this group.
_Check_return_
HRESULT
GroupItem::GetHeaderContent(_Outptr_ IControl** ppHeaderContent)
{
    IFCPTR_RETURN(ppHeaderContent);
    *ppHeaderContent = NULL;

    if(m_tpHeaderControl)
    {
        IFC_RETURN(m_tpHeaderControl.CopyTo(ppHeaderContent));
    }

    return S_OK;
}

// Helper to update m_tpHeaderControl. Attaches and/or detaches OnHeaderKeyDown.
_Check_return_
HRESULT
GroupItem::SetHeaderContentReference(_In_opt_ IControl* pHeaderContent)
{
    if (m_tpHeaderControl)
    {
        IFC_RETURN(m_tpHeaderControl.Cast<Control>()->remove_KeyDown(m_headerKeyDownToken));
    }

    if (pHeaderContent)
    {
        ctl::ComPtr<xaml_input::IKeyEventHandler> spHandler;

        spHandler.Attach(
            new ClassMemberEventHandler<
                GroupItem,
                IGroupItem,
                xaml_input::IKeyEventHandler,
                IInspectable,
                xaml_input::IKeyRoutedEventArgs>(this, &GroupItem::OnHeaderKeyDown));
        IFC_RETURN(static_cast<Control*>(pHeaderContent)->add_KeyDown(spHandler.Get(), &m_headerKeyDownToken));
    }

    SetPtrValue(m_tpHeaderControl, pHeaderContent);

    return S_OK;
}

// Invoked whenever application code or internal processes call
// ApplyTemplate.
IFACEMETHODIMP
GroupItem::OnApplyTemplate()
{
    ctl::ComPtr<xaml::IDependencyObject> spElementItemsControlAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spHeaderControlAsDO;
    ctl::ComPtr<IControl> spHeaderControl;

    IFC_RETURN(GroupItemGenerated::OnApplyTemplate());

    m_tpItemsControl.Clear();

    // Get the parts
    IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ItemsControl")).Get(), &spElementItemsControlAsDO));
    SetPtrValueWithQIOrNull(m_tpItemsControl, spElementItemsControlAsDO.Get());

    IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HeaderContent")).Get(), &spHeaderControlAsDO));
    spHeaderControl = spHeaderControlAsDO.AsOrNull<IControl>();
    IFC_RETURN(SetHeaderContentReference(spHeaderControl.Get()));

    if (m_tpItemsControl && m_tpGenerator)
    {
        ctl::ComPtr<IItemContainerGenerator> spParentGenerator;
        GroupStyle* pGroupStyleNoRef = NULL;

        IFC_RETURN(m_tpGenerator.Cast<ItemContainerGenerator>()->m_wrParent.As(&spParentGenerator));
        pGroupStyleNoRef = spParentGenerator.Cast<ItemContainerGenerator>()->m_tpGroupStyle.Cast<GroupStyle>();
        if (pGroupStyleNoRef)
        {
            ctl::ComPtr<IItemsPanelTemplate> spItemsPanel;

            IFC_RETURN(pGroupStyleNoRef->get_Panel(&spItemsPanel));
            IFC_RETURN(m_tpItemsControl->put_ItemsPanel(spItemsPanel.Get()));
        }
    }

    return S_OK;
}

// determines if this element should be transitioned using the passed in transition
IFACEMETHODIMP GroupItem::GetCurrentTransitionContext(
    _In_ INT layoutTickId,
    _Out_ ThemeTransitionContext* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);

    *pReturnValue = ThemeTransitionContext::None;
    // disabling the Group transitions for now, there are multiple issues needs to address:
    // 1, 469521 MoCo Grouping : Nested Transitions doesn't work
    // 2, Defining the Transition for groups
    // once both are fixed, this will be enabled back
    //if (m_pParentListViewBase)
    //{
    //    IFC_RETURN(m_pParentListViewBase->GetCurrentTransitionContext(layoutTickId, pReturnValue));
    //}
    //else
    //{
    //    // In case of Reset, by default we will return ContentTransition
    //    // There is bug that in case of reset, the parent ListView is null, and we can get correct transition from
    //    // the Parent
    //    // bug: 103077
    //    *pReturnValue = ThemeTransitionContext::ContentTransition;
    //}

    return S_OK;
}


// determines if mutations are going fast
IFACEMETHODIMP GroupItem::IsCollectionMutatingFast(
    _Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = FALSE;
    return S_OK;
}

IFACEMETHODIMP GroupItem::GetDropOffsetToRoot(
    _Out_ wf::Point* pReturnValue)
{
    ctl::ComPtr<IListViewBase> spParentListViewBase;

    IFCPTR_RETURN(pReturnValue);
    pReturnValue->X = 0;
    pReturnValue->Y = 0;

    IFC_RETURN(m_wrParentListViewBaseWeakRef.As(&spParentListViewBase));
    if (spParentListViewBase)
    {
        IFC_RETURN(spParentListViewBase.Cast<ListViewBase>()->GetDropOffsetToRoot(pReturnValue));
    }

    return S_OK;
}

_Check_return_
HRESULT
GroupItem::PrepareItemContainer(_In_ IInspectable* pGroupItem, _In_ ItemsControl* pItemsControl)
{
    ctl::ComPtr<IItemContainerGenerator> spParentGenerator;
    ctl::ComPtr<ICollectionViewGroup> spGroup;
    ctl::ComPtr<IInspectable> spGroupData;
    GroupStyle* pGroupStyleNoRef = nullptr;
    BOOLEAN bIsPropertyLocal = FALSE;

    auto ResetOnExit = wil::scope_exit([this] {
        SetCollectionViewGroup(NULL);
    });

    IFCPTR_RETURN(pItemsControl);

    if (!m_tpGenerator)
    {
        return S_OK; // user-declared GroupItem
    }

    IFC_RETURN(ctl::do_query_interface(spGroup, pGroupItem));
    IFC_RETURN(spGroup->get_Group(&spGroupData));

    m_wrParentListViewBaseWeakRef = nullptr;
    if (ctl::is<IListViewBase>(pItemsControl))
    {
        IFC_RETURN(ctl::AsWeak(ctl::as_iinspectable(pItemsControl), &m_wrParentListViewBaseWeakRef));
    }

    IFC_RETURN(m_tpGenerator.Cast<ItemContainerGenerator>()->m_wrParent.As(&spParentGenerator));

    // apply the container style
    pGroupStyleNoRef = spParentGenerator.Cast<ItemContainerGenerator>()->m_tpGroupStyle.Cast<GroupStyle>();
    if (pGroupStyleNoRef)
    {
        ctl::ComPtr<IStyle> spStyle;

        IFC_RETURN(pGroupStyleNoRef->get_ContainerStyle(&spStyle));

        // no ContainerStyle set, try ContainerStyleSelector
        if (!spStyle)
        {
            ctl::ComPtr<IStyleSelector> spStyleSelector;

            IFC_RETURN(pGroupStyleNoRef->get_ContainerStyleSelector(&spStyleSelector));
            if (spStyleSelector)
            {
                IFC_RETURN(spStyleSelector->SelectStyle(pGroupItem, this, &spStyle));
            }
        }

        // apply the style, if found
        if (spStyle)
        {
            // TODO: Set flag StyleSetFromGenerator
            IFC_RETURN(put_Style(spStyle.Get()));
        }
    }

    // forward the header template information

    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content),
        &bIsPropertyLocal));

    if (!bIsPropertyLocal)
    {
        IFC_RETURN(put_Content(spGroupData.Get()));
        SetCollectionViewGroup(spGroup.Get());
    }

    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_ContentTemplate),
        &bIsPropertyLocal));

    if (!bIsPropertyLocal && pGroupStyleNoRef)
    {
        ctl::ComPtr<IDataTemplate> spHeaderTemplate;

        IFC_RETURN(pGroupStyleNoRef->get_HeaderTemplate(&spHeaderTemplate));
        IFC_RETURN(put_ContentTemplate(spHeaderTemplate.Get()));
    }

    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_ContentTemplateSelector),
        &bIsPropertyLocal));

    if (!bIsPropertyLocal && pGroupStyleNoRef)
    {
        ctl::ComPtr<IDataTemplateSelector> spHeaderTemplateSelector;

        IFC_RETURN(pGroupStyleNoRef->get_HeaderTemplateSelector(&spHeaderTemplateSelector));
        IFC_RETURN(put_ContentTemplateSelector(spHeaderTemplateSelector.Get()));
    }
    
    ResetOnExit.release();
    return S_OK;
}

_Check_return_
HRESULT
GroupItem::ClearContainerForItem(_In_ IInspectable* pGroupItem)
{
    ctl::ComPtr<IItemContainerGenerator> spParentGenerator;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<ICollectionViewGroup> spGroup;
    ctl::ComPtr<IInspectable> spGroupData;
    GroupStyle* pGroupStyleNoRef = nullptr;

    bool areEqual = false;

    if (!m_tpGenerator)
    {
        // user-declared GroupItem
        return S_OK;
    }

    IFC_RETURN(ctl::do_query_interface(spGroup, pGroupItem));
    IFC_RETURN(spGroup->get_Group(&spGroupData));

    IFC_RETURN(m_tpGenerator.Cast<ItemContainerGenerator>()->m_wrParent.As(&spParentGenerator));

    IFC_RETURN(get_Content(&spContent));
    IFC_RETURN(PropertyValue::AreEqual(spGroupData.Get(), spContent.Get(), &areEqual));
    if (areEqual)
    {
        IFC_RETURN(ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content)));
        SetCollectionViewGroup(NULL);
    }

    pGroupStyleNoRef = spParentGenerator.Cast<ItemContainerGenerator>()->m_tpGroupStyle.Cast<GroupStyle>();
    if (pGroupStyleNoRef)
    {
        ctl::ComPtr<IDataTemplate> spContentTemplate;
        ctl::ComPtr<IDataTemplate> spHeaderTemplate;
        ctl::ComPtr<IDataTemplateSelector> spContentTemplateSelector;
        ctl::ComPtr<IDataTemplateSelector> spHeaderTemplateSelector;

        IFC_RETURN(get_ContentTemplate(&spContentTemplate));
        IFC_RETURN(pGroupStyleNoRef->get_HeaderTemplate(&spHeaderTemplate));
        if (spContentTemplate == spHeaderTemplate)
        {
            IFC_RETURN(ClearValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_ContentTemplate)));
        }

        IFC_RETURN(get_ContentTemplateSelector(&spContentTemplateSelector));
        IFC_RETURN(pGroupStyleNoRef->get_HeaderTemplateSelector(&spHeaderTemplateSelector));
        if (spContentTemplateSelector == spHeaderTemplateSelector)
        {
            IFC_RETURN(ClearValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_ContentTemplateSelector)));
        }
    }
    IFC_RETURN(m_tpGenerator->RemoveAll());

    return S_OK;
}

// Create GroupItemAutomationPeer to represent the GroupItem.
IFACEMETHODIMP GroupItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    ctl::ComPtr<xaml_automation_peers::IGroupItemAutomationPeer> spGroupItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IGroupItemAutomationPeerFactory> spGroupItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::GroupItemAutomationPeerFactory>::CreateActivationFactory());
    IFC_RETURN(spActivationFactory.As(&spGroupItemAPFactory));

    IFC_RETURN(spGroupItemAPFactory.Cast<GroupItemAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spGroupItemAutomationPeer));
    IFC_RETURN(spGroupItemAutomationPeer.CopyTo(ppAutomationPeer));

    return S_OK;
}

// Call ChangeSelectorItemsVisualState on our ItemsControl template part.
// This results in a call to ChangeVisualState on all child SelectorItems (including items inside another GroupItem),
// with optimizations for the virtualization provided by IOrientedVirtualizingPanel.
_Check_return_ HRESULT GroupItem::ChangeSelectorItemsVisualState(_In_ bool bUseTransitions)
{
    IFC_RETURN(m_tpItemsControl.Cast<ItemsControl>()->ChangeSelectorItemsVisualState(bUseTransitions));
    return S_OK;
}

void
GroupItem::SetCollectionViewGroup(_In_ ICollectionViewGroup *pGroup)
{
    SetPtrValue(m_tpCVG, pGroup);
}

_Check_return_
HRESULT
GroupItem::GetCollectionViewGroup(_Outptr_ ICollectionViewGroup **ppGroup)
{
    return (m_tpCVG.CopyTo(ppGroup));
}

// Obtains the index of the group this GroupItem is representing.
_Check_return_ HRESULT GroupItem::GetGroupIndex(_Out_ INT* pGroupIndex)
{
    ctl::ComPtr<IListViewBase> spIParentListViewBase;

    *pGroupIndex = -1;

    IFC_RETURN(m_wrParentListViewBaseWeakRef.As(&spIParentListViewBase));
    if (spIParentListViewBase)
    {
        ctl::ComPtr<ICollectionView> spCollectionView;

        IFC_RETURN(spIParentListViewBase.Cast<ListViewBase>()->get_CollectionView(&spCollectionView));
        if (spCollectionView)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;

            IFC_RETURN(spCollectionView->get_CollectionGroups(&spCollectionGroups));
            if (spCollectionGroups)
            {
                ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;
                UINT groupCount = 0;

                IFC_RETURN(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));
                IFC_RETURN(spCollectionGroupsAsV->get_Size(&groupCount));

                for (UINT i = 0; i < groupCount; i++)
                {
                    ctl::ComPtr<IInspectable> spCurrent;
                    bool areEqual = false;

                    IFC_RETURN(spCollectionGroupsAsV->GetAt(i, &spCurrent));
                    IFC_RETURN(ctl::are_equal(m_tpCVG.Get(), ctl::as_iinspectable(spCurrent.Get()), &areEqual));
                    if (areEqual)
                    {
                        *pGroupIndex = (INT)i;
                        break;
                    }

                }
            }
        }
    }

    return S_OK;
}
_Check_return_
HRESULT
GroupItem::GetTemplatedItemsControl(_Outptr_ xaml_controls::IItemsControl **ppItemsControl)
{
    return(m_tpItemsControl.CopyTo(ppItemsControl));
}

// GroupItem will be asked to hide if it is empty and GroupStyle.HidesIfEmpty is TRUE. In this case
// set Visibility to collapsed and add a listener to the generator to track if this group becomes non-empty.
_Check_return_ HRESULT GroupItem::Hide()
{
    ctl::ComPtr<xaml_primitives::IItemsChangedEventHandler> spItemsChangedHandler;

    memset(&m_itemsChangedToken, 0, sizeof(EventRegistrationToken));

    spItemsChangedHandler.Attach(
        new ClassMemberEventHandler<
            GroupItem,
            xaml_controls::IGroupItem,
            xaml_primitives::IItemsChangedEventHandler,
            IInspectable,
            xaml_primitives::IItemsChangedEventArgs>(this, &GroupItem::OnItemsChangedHandler));
    IFC_RETURN(m_tpGenerator->add_ItemsChanged(spItemsChangedHandler.Get(), &m_itemsChangedToken));

    IFC_RETURN(get_Visibility(&m_previousVisibility));
    IFC_RETURN(put_Visibility(xaml::Visibility_Collapsed));
    m_bWasHidden = TRUE;

    return S_OK;
}

_Check_return_ HRESULT GroupItem::OnItemsChangedHandler(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;
    ctl::ComPtr<IInspectable> spItem;
    UINT nGroupSize = 0;

    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer, spItem.GetAddressOf()));
    spGroup = spItem.AsOrNull<xaml_data::ICollectionViewGroup>();

    // Group may be NULL if this container was unlinked e.g. OnRefresh.
    if (spGroup)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroupItems;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupView;

        IFC_RETURN(spGroup->get_GroupItems(&spGroupItems));
        IFC_RETURN(spGroupItems.As(&spGroupView));
        IFC_RETURN(spGroupView->get_Size(&nGroupSize));
    }

    // If the group becomes non-empty, un-hide the UI by removing the placeholder.
    if (nGroupSize > 0)
    {
        if (m_itemsChangedToken.value)
        {
            if (m_tpGenerator)
            {
                // Detach the generator's event handler and reset the generator.
                IFC_RETURN(m_tpGenerator->remove_ItemsChanged(m_itemsChangedToken));
            }
            memset(&m_itemsChangedToken, 0, sizeof(EventRegistrationToken));
        }

        // Restore the previous visibility.
        IFC_RETURN(put_Visibility(m_previousVisibility));
        m_bWasHidden = FALSE;
    }

    return S_OK;
}

// Places focus on the group header, if possible. Note the default GroupItem style has a non-focusable HeaderContent
// part, meaning that there needs to be focusable content within HeaderContent for this to work.
_Check_return_ HRESULT GroupItem::FocusHeader(_In_ xaml::FocusState focusState, _Out_ BOOLEAN* pDidSetFocus)
{
    *pDidSetFocus = FALSE;

    if (m_tpHeaderControl)
    {
        IFC_RETURN(m_tpHeaderControl.AsOrNull<IUIElement>()->Focus(focusState, pDidSetFocus));
    }
    
    return S_OK;
}

// Called when the GroupItem receives focus.
IFACEMETHODIMP GroupItem::OnGotFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Programmatic;

    IFC_RETURN(GroupItemGenerated::OnGotFocus(pArgs));

    IFC_RETURN(pArgs->get_OriginalSource(&spOriginalSource));
    if (spOriginalSource)
    {
        ctl::ComPtr<IUIElement> spFocusedElement;

        spFocusedElement = spOriginalSource.AsOrNull<IUIElement>();
        if (spFocusedElement)
        {
            IFC_RETURN(spFocusedElement->get_FocusState(&focusState));
        }
    }

    IFC_RETURN(HasFocus(&hasFocus));
    IFC_RETURN(FocusChanged(hasFocus, focusState));

    return S_OK;
}

// Called when the GroupItem loses focus.
IFACEMETHODIMP GroupItem::OnLostFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC_RETURN(GroupItemGenerated::OnLostFocus(pArgs));

    IFC_RETURN(pArgs->get_OriginalSource(&spOriginalSource));
    if (spOriginalSource)
    {
        ctl::ComPtr<IUIElement> spFocusedElement;

        spFocusedElement = spOriginalSource.AsOrNull<IUIElement>();
        if (spFocusedElement)
        {
            IFC_RETURN(spFocusedElement->get_FocusState(&focusState));
        }
    }

    IFC_RETURN(HasFocus(&hasFocus));
    IFC_RETURN(FocusChanged(hasFocus, focusState));

    return S_OK;
}

// Update our parent ListView when group focus changes so it can manage
// the currently focused header, if appropriate.
_Check_return_ HRESULT GroupItem::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ xaml::FocusState howFocusChanged)
{
    ctl::ComPtr<IListViewBase> spIParentListViewBase;
    BOOLEAN isFocusedByPointer = !(xaml::FocusState_Keyboard == howFocusChanged ||
                                   xaml::FocusState_Programmatic == howFocusChanged);

    IFC_RETURN(m_wrParentListViewBaseWeakRef.As(&spIParentListViewBase));
    if (spIParentListViewBase)
    {
        ListViewBase* pParentListViewBaseNoRef = spIParentListViewBase.Cast<ListViewBase>();
        // If the parent ListView doesn't have a focused group, then focus
        // is coming from outside of the ListView.
        // If this is the case and we're not being focused by pointer, ask our LVB to
        // forward focus to the correct element.
        if (!isFocusedByPointer && !pParentListViewBaseNoRef->HasFocusedGroup())
        {
            BOOLEAN headerHasFocus = FALSE;
            if (m_tpHeaderControl)
            {
                IFC_RETURN(m_tpHeaderControl.Cast<Control>()->HasFocus(&headerHasFocus));
            }
            // If this GroupItem is obtaining focus from outside the
            // ListView, then it's merely the first focusable header and we
            // really want to focus the last focused group header.  Pass this
            // notification along to our parent who will focus the last
            // focused header.  (Note: Our parent will usually have
            // IsTabStop=False so it won't receive its own GotFocus and
            // LostFocus notifications.)
            IFC_RETURN(pParentListViewBaseNoRef->OnGroupFocusChanged(hasFocus, headerHasFocus, howFocusChanged));
        }
        else if (hasFocus)
        {
            IFC_RETURN(pParentListViewBaseNoRef->GroupItemFocused(this));
        }
        else
        {
            IFC_RETURN(pParentListViewBaseNoRef->GroupItemUnfocused(this));
        }
    }

    return S_OK;
}

// Unloads all containers to main generator container recycling queue
_Check_return_ HRESULT GroupItem::Recycle()
{
    if (m_tpItemsControl)
    {
        ctl::ComPtr<IPanel> spItemsHost;

        IFC_RETURN(m_tpItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHost));

        // we should ignore transitions during recycling
        if (spItemsHost)
        {
            IFC_RETURN(spItemsHost.Cast<Panel>()->put_IsIgnoringTransitions(TRUE));
        }
    }

    if (m_bWasHidden)
    {
        if (m_itemsChangedToken.value != 0)
        {
            if (m_tpGenerator)
            {
                // Detach the generator's event handler and reset the generator.
                IFC_RETURN(m_tpGenerator->remove_ItemsChanged(m_itemsChangedToken));
            }
            memset(&m_itemsChangedToken, 0, sizeof(EventRegistrationToken));
        }
        // Restore the previous visibility.
        IFC_RETURN(put_Visibility(m_previousVisibility));

        // initialize to initial state as c~tor does
        m_bWasHidden = FALSE;
        m_previousVisibility = xaml::Visibility_Collapsed;
    }

    // unload containers and clear items host panel
    if (m_tpGenerator)
    {
        IFC_RETURN(m_tpGenerator.Cast<ItemContainerGenerator>()->Refresh());
    }

    return S_OK;
}

// Provides the behavior for the Arrange pass of layout.  Classes
// can override this method to define their own Arrange pass
// behavior.
IFACEMETHODIMP GroupItem::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    IFC_RETURN(GroupItemGenerated::ArrangeOverride(arrangeSize, returnValue));

    if (m_tpItemsControl)
    {
        ctl::ComPtr<IPanel> spItemsHost;

        IFC_RETURN(m_tpItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHost));

        if (spItemsHost)
        {
            IFC_RETURN(spItemsHost.Cast<Panel>()->put_IsIgnoringTransitions(FALSE));
        }
    }

    return S_OK;
}

void GroupItem::SetGenerator(
    _In_opt_ xaml_controls::IItemContainerGenerator* const pGenerator)
{
    if (pGenerator)
    {
        SetPtrValue(m_tpGenerator, pGenerator);
    }
    else
    {
        m_tpGenerator.Clear();
    }
}
