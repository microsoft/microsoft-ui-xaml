// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xcpmath.h"
#include "MenuFlyoutPresenter.g.h"
#include "ItemCollection.g.h"
#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutSubItem.g.h"
#include "ToggleMenuFlyoutItem.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutPresenterAutomationPeer.g.h"
#include "MenuFlyoutPresenterTemplateSettings.g.h"
#include "Window.g.h"
#include "DispatcherTimer.g.h"
#include "ToolTipService.g.h"
#include "Popup.g.h"
#include "VisualTreeHelper.h"
#include "ResourceDictionary.g.h"
#include "AutomationProperties.h"
#include "InputServices.h"
#include "ElevationHelper.h"
#include "ThemeShadow.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

MenuFlyoutPresenter::MenuFlyoutPresenter()
    : m_iFocusedIndex(-1)
    , m_containsToggleItems(false)
    , m_containsIconItems(false)
    , m_containsItemsWithKeyboardAcceleratorText(false)
    , m_animationInProgress(false)
    , m_isSubPresenter(false)
    , m_mostRecentPlacement(FlyoutBase::MajorPlacementMode::Bottom)
{
}

_Check_return_ HRESULT
MenuFlyoutPresenter::PrepareState()
{
    IFC_RETURN(MenuFlyoutPresenterGenerated::PrepareState());

    ctl::ComPtr<MenuFlyoutPresenterTemplateSettings> spTemplateSettings;
    IFC_RETURN(ctl::make(&spTemplateSettings));
    IFC_RETURN(put_TemplateSettings(spTemplateSettings.Get()));

    EventRegistrationToken loadedToken;
    ctl::ComPtr<xaml::IRoutedEventHandler> loadedEventHandler;

    loadedEventHandler.Attach(
        new ClassMemberEventHandler<
        MenuFlyoutPresenter,
        xaml::Controls::IMenuFlyoutPresenter,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &MenuFlyoutPresenter::OnLoaded, true /* subscribingToSelf */));

    IFC_RETURN(add_Loaded(loadedEventHandler.Get(), &loadedToken));

    return S_OK;
}

// Responds to the KeyDown event.
IFACEMETHODIMP
MenuFlyoutPresenter::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        wsy::VirtualKey key = wsy::VirtualKey_None;
        IFC(pArgs->get_Key(&key));
        IFC(KeyPress::MenuFlyoutPresenter::KeyDown<MenuFlyoutPresenter>(key, this, &handled));
        IFC(pArgs->put_Handled(handled));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MenuFlyoutPresenter::HandleUpOrDownKey(
    BOOLEAN isDownKey)
{
    return CycleFocus(isDownKey, xaml::FocusState_Keyboard);
}

_Check_return_ HRESULT
MenuFlyoutPresenter::CycleFocus(
    BOOLEAN shouldCycleDown,
    xaml::FocusState focusState)
{
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;

    // Ensure the initial focus index to validate m_iFocusedIndex when the focused item
    // is set by application's code like as MenuFlyout Opened event.
    IFC_RETURN(EnsureInitialFocusIndex());

    INT originalFocusedIndex = m_iFocusedIndex;

    ctl::ComPtr<MenuFlyout> parentFlyout;
    IFC_RETURN(GetParentMenuFlyout(&parentFlyout));

    // We should wrap around at the bottom or the top of the presenter if the user isn't using a gamepad or remote.
    bool shouldWrap = parentFlyout != nullptr ? (parentFlyout->GetInputDeviceTypeUsedToOpen() != DirectUI::InputDeviceType::GamepadOrRemote) : true;

    IFC_RETURN(get_Items(&spItems));
    IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    // Determine direction of index movement based on the Up/Down key.
    INT deltaIndex = shouldCycleDown ? 1 : -1;

    // Set index by moving deltaIndex amount from the current focused item.
    INT index = m_iFocusedIndex + deltaIndex;

    // We have two locations where we want to wrap, so we'll encapsulate the wrapping behavior in a function object that we can call.
    std::function<int(int)> wrapIndexIfNeeded =
        [&](int indexToWrap) -> int
        {
            if (shouldWrap)
            {
                if (indexToWrap < 0)
                {
                    indexToWrap = static_cast<INT>(nCount) - 1;
                }
                else if (indexToWrap >= static_cast<INT>(nCount))
                {
                    indexToWrap = 0;
                }
            }

            return indexToWrap;
        };

    // If there is no item focused right now, then set index to 0 for Down key or to n-1 for Up key.
    // Otherwise, if we should be wrapping, we'll do an initial check for whether we should wrap before we enter the loop.
    if (m_iFocusedIndex == -1)
    {
        index = shouldCycleDown ? 0 : static_cast<INT>(nCount) - 1;

        // If the focused index is -1, then our value of -1 for originalFocusedIndex will not successfully stop the loop.
        // In this case, we'll make originalFocusedIndex one step in the opposite direction from the initial index,
        // so that way the loop will go all the way through the list of items before stopping.
        originalFocusedIndex = wrapIndexIfNeeded(index - deltaIndex);
    }
    else
    {
        index = wrapIndexIfNeeded(index);
    }

    // We need to examine all items with indices [0, m_iFocusedIndex) or (m_iFocusedIndex, nCount-1] for Down/Up keys.
    // While index is within the range, we keep going through the item list until we are successfully able to focus an item,
    // at which point we update the m_iFocusedIndex and break out of the loop.
    while (0 <= index && index < static_cast<INT>(nCount))
    {
        ctl::ComPtr<IInspectable> spItemAsIInspectable = NULL;
        IFC_RETURN(spItems.Cast<ItemCollection>()->GetAt(index, &spItemAsIInspectable));

        bool isFocusable = false;

        // We determine whether the item is a focusable MenuFlyoutItem or MenuFlyoutSubItem because we want to exclude MenuSeparators here.
        ctl::ComPtr<MenuFlyoutItem> spItem = NULL;
        spItem = spItemAsIInspectable.AsOrNull<IMenuFlyoutItem>().Cast<MenuFlyoutItem>();
        if (spItem)
        {
            IFC_RETURN(CoreImports::UIElement_IsFocusable(static_cast<CUIElement *>(spItem.Cast<MenuFlyoutItem>()->GetHandle()), &isFocusable));
        }
        else
        {
            ctl::ComPtr<MenuFlyoutSubItem> spSubItem;
            spSubItem = spItemAsIInspectable.AsOrNull<IMenuFlyoutSubItem>().Cast<MenuFlyoutSubItem>();
            if (spSubItem)
            {
                IFC_RETURN(CoreImports::UIElement_IsFocusable(static_cast<CUIElement *>(spSubItem.Cast<MenuFlyoutSubItem>()->GetHandle()), &isFocusable));
            }
        }

        // If the item is focusable, move the focus to it, update the m_iFocusedIndex and break out of the loop.
        if (isFocusable)
        {
            auto spSubItemAsUIE = spItemAsIInspectable.AsOrNull<IUIElement>();
            if (spSubItemAsUIE)
            {
                BOOLEAN wasFocused = FALSE;
                IFC_RETURN(spSubItemAsUIE->Focus(focusState, &wasFocused));

                m_iFocusedIndex = index;
                break;
            }
        }

        // If we've gone all the way around the list of items and still have not found a suitable focus candidate,
        // then we'll stop - there's nothing else for us to do.
        if (index == originalFocusedIndex)
        {
            break;
        }

        index += deltaIndex;

        // If we should be wrapping, then we'll perform the wrap at this point.
        index = wrapIndexIfNeeded(index);
    }
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::HandleKeyDownLeftOrEscape()
{
    return CloseSubMenu();
}

IFACEMETHODIMP
MenuFlyoutPresenter::PrepareContainerForItemOverride(
    _In_ xaml::IDependencyObject* pElement,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IMenuFlyoutItemBase> spMenuFlyoutItemBase;

    IFC(MenuFlyoutPresenterGenerated::PrepareContainerForItemOverride(pElement, pItem));

    IFC(ctl::do_query_interface(spMenuFlyoutItemBase, pElement));
    IFC(spMenuFlyoutItemBase.Cast<MenuFlyoutItemBase>()->SetParentMenuFlyoutPresenter(this));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
MenuFlyoutPresenter::ClearContainerForItemOverride(
    _In_ xaml::IDependencyObject* pElement,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IMenuFlyoutItemBase> spMenuFlyoutItemBase;

    IFC(MenuFlyoutPresenterGenerated::ClearContainerForItemOverride(pElement, pItem));

    IFC(ctl::do_query_interface(spMenuFlyoutItemBase, pElement));
    IFC(spMenuFlyoutItemBase.Cast<MenuFlyoutItemBase>()->SetParentMenuFlyoutPresenter(nullptr));

Cleanup:
    RRETURN(hr);
}

// Get the parent MenuFlyout.
_Check_return_ HRESULT
MenuFlyoutPresenter::GetParentMenuFlyout(
    _Outptr_result_maybenull_ MenuFlyout** ppParentMenuFlyout)
{
    *ppParentMenuFlyout = NULL;

    if (m_wrParentMenuFlyout)
    {
        ctl::ComPtr<MenuFlyout> parent;
        IFC_RETURN(m_wrParentMenuFlyout.As(&parent));
        *ppParentMenuFlyout = parent.Detach();
    }

    return S_OK;
}

// Sets the parent MenuFlyout.
_Check_return_ HRESULT MenuFlyoutPresenter::SetParentMenuFlyout(
    _In_ MenuFlyout* pParentMenuFlyout)
{
    IFC_RETURN(ctl::AsWeak(ctl::as_iinspectable(pParentMenuFlyout), &m_wrParentMenuFlyout));
    return S_OK;
}

// Called when the ItemsSource property changes.
_Check_return_ HRESULT
MenuFlyoutPresenter::OnItemsSourceChanged(
    _In_ IInspectable* pNewValue)
{
    IFC_RETURN(MenuFlyoutPresenterGenerated::OnItemsSourceChanged(pNewValue));

    m_iFocusedIndex = -1;
    m_containsToggleItems = false;
    m_containsIconItems = false;
    m_containsItemsWithKeyboardAcceleratorText = false;

    if (pNewValue)
    {
        ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spItems;
        UINT nCount = 0;

        spItems = ctl::query_interface_cast<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>>(pNewValue);
        if (!spItems)
        {
            // MenuFlyoutPresenter could be used outside of MenuFlyout, but at this time
            // we don't support that usage.  If a customer is using MenuFlyoutPresenter
            // independently and uses an ItemsSource that is not an IVector<MenuFlyoutItemBase>,
            // we throw E_INVALIDARG to indicate that this usage is invalid.
            // If we decide to allow this usage, we need to override and implement
            // IsItemItsOwnContainerOverride() and GetContainerForItemOverride().
            //
            // GetContainerForItemOverride() is tricky since MenuFlyoutPresenter supports 3 different
            // kinds of children (MenuFlyoutSeparator, MenuFlyoutItem, ToggleMenuFlyoutItem),
            // so we are punting on that scenario for now.
            IFC_RETURN(E_INVALIDARG);
        }

        // MenuFlyoutItem's alignment changes based on the layout of other MenuFlyoutItems.
        // This check looks through all MenuFlyoutItems in our items source and checks for
        // ToggleMenuFlyoutItems and the presence of Icons, which can change the layout of
        // all MenuFlyoutItems in the presenter.
        IFC_RETURN(spItems->get_Size(&nCount));
        for (UINT i = 0; i < nCount; ++i)
        {
            ctl::ComPtr<IMenuFlyoutItemBase> item;
            ctl::ComPtr<IMenuFlyoutItem> itemAsMenuItem;
            ctl::ComPtr<IMenuFlyoutSubItem> itemAsMenuSubItem;
            ctl::ComPtr<IIconElement> iconElement;
            wrl_wrappers::HString keyboardAcceleratorText;

            IFC_RETURN(spItems->GetAt(i, &item));

            // To prevent casting the same item more than we need to, each cast is conditional
            // on the previous one failing. This way we only cast each item as many times as we
            // need to.
            itemAsMenuItem = item.AsOrNull<IMenuFlyoutItem>();
            if (itemAsMenuItem)
            {
                m_containsToggleItems = m_containsToggleItems || itemAsMenuItem.Cast<MenuFlyoutItem>()->HasToggle();

                IFC_RETURN(itemAsMenuItem.Cast<MenuFlyoutItem>()->get_Icon(&iconElement));
                m_containsIconItems = m_containsIconItems || iconElement;

                IFC_RETURN(itemAsMenuItem.Cast<MenuFlyoutItem>()->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorText.ReleaseAndGetAddressOf()));
                m_containsItemsWithKeyboardAcceleratorText = m_containsItemsWithKeyboardAcceleratorText || !keyboardAcceleratorText.IsEmpty();
            }
            else
            {
                itemAsMenuSubItem = item.AsOrNull<IMenuFlyoutSubItem>();
                if (itemAsMenuSubItem)
                {
                    IFC_RETURN(itemAsMenuSubItem.Cast<MenuFlyoutSubItem>()->get_Icon(&iconElement));
                    m_containsIconItems = m_containsIconItems || iconElement;
                }
            }

            if (m_containsIconItems && m_containsToggleItems && m_containsItemsWithKeyboardAcceleratorText)
            {
                break;
            }
        }

        IFC_RETURN(UpdateTemplateSettings());
    }

    return S_OK;
}

// Create MenuFlyoutPresenterAutomationPeer to represent the MenuFlyoutPresenter.
IFACEMETHODIMP MenuFlyoutPresenter::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IMenuFlyoutPresenterAutomationPeer> spMenuFlyoutPresenterAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IMenuFlyoutPresenterAutomationPeerFactory> spMenuFlyoutPresenterAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::MenuFlyoutPresenterAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spMenuFlyoutPresenterAPFactory));

    IFC(spMenuFlyoutPresenterAPFactory.Cast<MenuFlyoutPresenterAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spMenuFlyoutPresenterAutomationPeer));
    IFC(spMenuFlyoutPresenterAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyoutPresenter::UpdateVisualStateForPlacement(
    FlyoutBase::MajorPlacementMode placement)
{
    HRESULT hr = S_OK;

    m_mostRecentPlacement = placement;
    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyoutPresenter::ResetVisualState()
{
    HRESULT hr = S_OK;
    BOOLEAN isSuccess = FALSE;

    IFC(GoToState(FALSE, L"None", &isSuccess));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyoutPresenter::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN isSuccess = FALSE;

    IFC(MenuFlyoutPresenterGenerated::ChangeVisualState(bUseTransitions));

    if (m_mostRecentPlacement == FlyoutBase::MajorPlacementMode::Left)
    {
        m_animationInProgress = m_epLeftLandscapeCompletedHandler;
        IFC(GoToState(bUseTransitions, L"LeftLandscape", &isSuccess));
    }
    else if (m_mostRecentPlacement == FlyoutBase::MajorPlacementMode::Right)
    {
        m_animationInProgress = m_epRightLandscapeCompletedHandler;
        IFC(GoToState(bUseTransitions, L"RightLandscape", &isSuccess));
    }
    else if (m_mostRecentPlacement == FlyoutBase::MajorPlacementMode::Top)
    {
        m_animationInProgress = m_epTopPortraitCompletedHandler;
        IFC(GoToState(bUseTransitions, L"TopPortrait", &isSuccess));
    }
    else
    {
        m_animationInProgress = m_epBottomPortraitCompletedHandler;
        IFC(GoToState(bUseTransitions, L"BottomPortrait", &isSuccess));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutPresenter::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;

    IFC(MenuFlyoutPresenterGenerated::OnApplyTemplate());

    // Get the ScrollViewer template part
    m_tpScrollViewer.Clear();
    IFC(GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"MenuFlyoutPresenterScrollViewer"), spScrollViewer.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpScrollViewer, spScrollViewer.Get());

    // Apply a shadow
    BOOLEAN isDefaultShadowEnabled;
    IFC(get_IsDefaultShadowEnabled(&isDefaultShadowEnabled));
    if (isDefaultShadowEnabled)
    {
        if (CThemeShadow::IsDropShadowMode())
        {
            // In drop shadow mode, we apply the shadow to the MenuFlyoutPresenter, otherwise the shadow is clipped.
            IFC(ApplyElevationEffect(this, GetDepth()));
        }
        else
        {
            ctl::ComPtr<IDependencyObject> spChild;
            IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChild));
            auto spChildAsUIE = spChild.AsOrNull<IUIElement>();

            if (spChildAsUIE)
            {
                IFC(ApplyElevationEffect(spChildAsUIE.Get(), GetDepth()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyoutPresenter::GetOwnerName(_Out_ HSTRING* pName)
{
    ctl::ComPtr<MenuFlyout> parentMenuFlyout;
    IFC_RETURN(GetParentMenuFlyout(&parentMenuFlyout));

    if (parentMenuFlyout)
    {
        ctl::ComPtr<IInspectable> spName;

        IFC_RETURN(parentMenuFlyout->GetValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name),
            &spName));

        IFC_RETURN(ctl::do_get_value(*pName, spName.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::AttachEntranceAnimationCompleted(
    _In_reads_(nStateNameLength + 1) _Null_terminated_ const WCHAR* pszStateName,
    XUINT32 nStateNameLength,
    _Outptr_result_maybenull_ xaml_animation::ITimeline** ppTimeline,
    _In_ ctl::EventPtr<TimelineCompletedEventCallback>* pCompletedEvent)
{
    HRESULT hr = S_OK;
    BOOLEAN found = FALSE;
    ctl::ComPtr<xaml::IVisualState> spState;

    IFCPTR(ppTimeline);
    *ppTimeline = NULL;

    IFC(VisualStateManager::TryGetState(this, pszStateName, nullptr, &spState, &found));

    if (found && spState)
    {
        ctl::ComPtr<IStoryboard> spStoryboard;

        IFC(spState->get_Storyboard(&spStoryboard));
        if (spStoryboard)
        {
           ctl::ComPtr<xaml_animation::ITimeline> spTimeline;
           IFC(spStoryboard.As<ITimeline>(&spTimeline));

           IFC(pCompletedEvent->AttachEventHandler(spTimeline.Get(),
               std::bind(&MenuFlyoutPresenter::OnEntranceAnimationCompleted, this, _1, _2)));

           IFC(spTimeline.CopyTo(ppTimeline));
       }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MenuFlyoutPresenter::DetachEntranceAnimationCompletedHandlers()
{
    HRESULT hr = S_OK;

    if (m_epTopPortraitCompletedHandler)
    {
        ASSERT(m_tpTopPortraitTimeline);
        IFC(m_epTopPortraitCompletedHandler.DetachEventHandler(m_tpTopPortraitTimeline.Get()));
        m_tpTopPortraitTimeline.Clear();
    }

    if (m_epBottomPortraitCompletedHandler)
    {
        ASSERT(m_tpBottomPortraitTimeline);
        IFC(m_epBottomPortraitCompletedHandler.DetachEventHandler(m_tpBottomPortraitTimeline.Get()));
        m_tpBottomPortraitTimeline.Clear();
    }

    if (m_epLeftLandscapeCompletedHandler)
    {
        ASSERT(m_tpLeftLandscapeTimeline);
        IFC(m_epLeftLandscapeCompletedHandler.DetachEventHandler(m_tpLeftLandscapeTimeline.Get()));
        m_tpLeftLandscapeTimeline.Clear();
    }

    if (m_epRightLandscapeCompletedHandler)
    {
        ASSERT(m_tpRightLandscapeTimeline);
        IFC(m_epRightLandscapeCompletedHandler.DetachEventHandler(m_tpRightLandscapeTimeline.Get()));
        m_tpRightLandscapeTimeline.Clear();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MenuFlyoutPresenter::OnEntranceAnimationCompleted(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN returnValue = FALSE;

    m_animationInProgress = false;

    IFC(Focus(xaml::FocusState_Programmatic, &returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
MenuFlyoutPresenter::OnPointerExited(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    BOOLEAN handled = FALSE;

    IFC_RETURN(MenuFlyoutPresenterGenerated::OnPointerExited(pArgs));

    IFC_RETURN(pArgs->get_Handled(&handled));

    if (!handled)
    {
        ctl::ComPtr<xaml_input::IPointer> spPointer;
        auto pointerDeviceType = mui::PointerDeviceType_Touch;

        IFC_RETURN(pArgs->get_Pointer(&spPointer));
        IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));

        if (mui::PointerDeviceType_Mouse == pointerDeviceType && !m_isSubPresenter)
        {
            bool isHitVerticalScrollBarOrSubPresenter = false;
            ctl::ComPtr<IMenuPresenter> subPresenter;

            // Hit test the current position for the vertical ScrollBar and the sub presenter.
            // Close the existing sub presenter if the current mouse position isn't hit
            // the vertical ScrollBar nor sub presenter.

            IFC_RETURN(get_SubPresenter(&subPresenter));

            if (subPresenter)
            {
                wf::Point clientLogicalPointerPosition = {};
                ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
                ctl::ComPtr<IUIElement> spSubPresenterAsUIE;

                IFC_RETURN(subPresenter.As(&spSubPresenterAsUIE));

                IFC_RETURN(pArgs->GetCurrentPoint(nullptr /* relativeTo*/, &spPointerPoint));
                IFC_RETURN(spPointerPoint->get_Position(&clientLogicalPointerPosition));

                if (m_tpScrollViewer)
                {
                    ctl::ComPtr<IUIElement> spVerticalScrollBarAsUE;
                    ctl::ComPtr<IControl> spScrollViewerAsControl;
                    ctl::ComPtr<IDependencyObject> spVerticalScrollBarAsDO;

                    IFC_RETURN(m_tpScrollViewer.As<IControl>(&spScrollViewerAsControl));
                    IFC_RETURN(static_cast<Control *>(spScrollViewerAsControl.Get())->GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalScrollBar")).Get(), &spVerticalScrollBarAsDO));
                    IFC_RETURN(spVerticalScrollBarAsDO.As<IUIElement>(&spVerticalScrollBarAsUE));

                    if (spSubPresenterAsUIE || spVerticalScrollBarAsUE)
                    {
                        BOOLEAN hasCurrent = FALSE;

                        ctl::ComPtr<wfc::IIterable<xaml::UIElement*>> spElements;
                        ctl::ComPtr<wfc::IIterator<xaml::UIElement*>> spIterator;

                        IFC_RETURN(VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(clientLogicalPointerPosition, spVerticalScrollBarAsUE.Get(), TRUE /* includeAllElements */, &spElements));
                        IFC_RETURN(spElements->First(&spIterator));
                        IFC_RETURN(spIterator->get_HasCurrent(&hasCurrent));

                        while (hasCurrent)
                        {
                            ctl::ComPtr<xaml::IUIElement> spElement;

                            IFC_RETURN(spIterator->get_Current(&spElement));

                            CDependencyObject *pElementAsCDO = static_cast<UIElement *>(spElement.Get())->GetHandle();

                            if ((pElementAsCDO->GetTypeIndex() == KnownTypeIndex::ScrollBar && spVerticalScrollBarAsUE.Get() == spElement.Get()) ||
                                (spSubPresenterAsUIE.Get() == spElement.Get()))
                            {
                                isHitVerticalScrollBarOrSubPresenter = true;
                                break;
                            }

                            IFC_RETURN(spIterator->MoveNext(&hasCurrent));
                        }

                        if (!isHitVerticalScrollBarOrSubPresenter)
                        {
                            IFC_RETURN(VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(clientLogicalPointerPosition, spSubPresenterAsUIE.Get(), TRUE /* includeAllElements */, &spElements));
                            IFC_RETURN(spElements->First(&spIterator));
                            IFC_RETURN(spIterator->get_HasCurrent(&hasCurrent));

                            while (hasCurrent)
                            {
                                ctl::ComPtr<xaml::IUIElement> spElement;

                                IFC_RETURN(spIterator->get_Current(&spElement));

                                CDependencyObject *pElementAsCDO = static_cast<UIElement *>(spElement.Get())->GetHandle();

                                if ((pElementAsCDO->GetTypeIndex() == KnownTypeIndex::ScrollBar && spVerticalScrollBarAsUE.Get() == spElement.Get()) ||
                                    (spSubPresenterAsUIE.Get() == spElement.Get()))
                                {
                                    isHitVerticalScrollBarOrSubPresenter = true;
                                    break;
                                }

                                IFC_RETURN(spIterator->MoveNext(&hasCurrent));
                            }
                        }
                    }
                }

                // The opened MenuFlyoutSubItem won't to be closed if the mouse position is
                // on the vertical ScrollBar or sub presenter.
                if (!isHitVerticalScrollBarOrSubPresenter)
                {
                    XRECTF_RB subPresenterBoundsLogical = {};
                    IFC_RETURN(GetSubPresenterBounds(spSubPresenterAsUIE.Get(), &subPresenterBoundsLogical));

                    if (!DoesRectContainPointInclusive(subPresenterBoundsLogical, { clientLogicalPointerPosition.X, clientLogicalPointerPosition.Y }))
                    {
                        IFC_RETURN(DelayCloseMenuFlyoutSubItem());
                    }
                }
            }
        }

        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::GetSubPresenterBounds(_In_ IUIElement* pSubPresenterAsUIE, _Out_ XRECTF_RB *subPresenterBounds)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<UIElement *>(pSubPresenterAsUIE)->GetHandle());
    IFC_RETURN(coreElement->GetGlobalBoundsLogical(subPresenterBounds));
    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutPresenter::OnPointerEntered(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    BOOLEAN handled = FALSE;

    IFC_RETURN(MenuFlyoutPresenterGenerated::OnPointerEntered(pArgs));

    IFC_RETURN(pArgs->get_Handled(&handled));

    if (!handled)
    {
        ctl::ComPtr<xaml_input::IPointer> spPointer;
        auto pointerDeviceType = mui::PointerDeviceType_Touch;

        IFC_RETURN(pArgs->get_Pointer(&spPointer));
        IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));

        if (mui::PointerDeviceType_Mouse == pointerDeviceType && m_isSubPresenter)
        {
            IFC_RETURN(CancelCloseMenuFlyoutSubItem());
            ctl::ComPtr<ISubMenuOwner> owner;
            IFC_RETURN(get_Owner(&owner));

            if (owner)
            {
                // When the mouse enters a MenuFlyoutPresenter that is a sub menu
                // we have to tell the parent presenter to cancel any plans it had
                // to close this sub MenuFlyoutPresenter.
                IFC_RETURN(owner->CancelCloseSubMenu());
            }
        }

        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::GetPlainText(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IDependencyObject> ownerFlyout;
    HSTRING automationName = nullptr;

    IFC_RETURN(m_wrParentMenuFlyout.As(&ownerFlyout));

    if (ownerFlyout)
    {
        // If an automation name is set on the parent flyout, we'll use that as our plain text.
        // Otherwise, we'll report the default plain text.
        IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(ownerFlyout.Get(), &automationName));
    }

    if (automationName != nullptr)
    {
        *strPlainText = automationName;
    }
    else
    {
        // If we have no title, we'll fall back to the default implementation,
        // which retrieves our content as plain text (e.g., if our content is a string,
        // it returns that; if our content is a TextBlock, it returns its Text value, etc.)
        IFC_RETURN(MenuFlyoutPresenterGenerated::GetPlainText(strPlainText));

        // If we get the plain text from the content, then we want to truncate it,
        // in case the resulting automation name is very long.
        IFC_RETURN(Popup::TruncateAutomationName(strPlainText));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::CloseSubMenuImpl()
{
    ctl::ComPtr<IMenuPresenter> subPresenter;

    IFC_RETURN(m_wrSubPresenter.As(&subPresenter));
    if (subPresenter)
    {
        IFC_RETURN(subPresenter->CloseSubMenu());
    }

    ctl::ComPtr<ISubMenuOwner> owner;
    IFC_RETURN(m_wrOwner.As(&owner));

    if (owner)
    {
        IFC_RETURN(owner->CloseSubMenu());
    }

    // Reset the focused index not to cached the previous focused index when
    // the sub menu is opened in the next time
    m_iFocusedIndex = -1;

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::DelayCloseMenuFlyoutSubItem()
{
    ctl::ComPtr<IMenuPresenter> subMenuPresenter;

    IFC_RETURN(m_wrSubPresenter.As(&subMenuPresenter));
    if (subMenuPresenter)
    {
        ctl::ComPtr<ISubMenuOwner> subMenuOwner;
        IFC_RETURN(subMenuPresenter->get_Owner(&subMenuOwner));

        if (subMenuOwner)
        {
            IFC_RETURN(subMenuOwner->DelayCloseSubMenu());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::CancelCloseMenuFlyoutSubItem()
{
    ctl::ComPtr<IMenuPresenter> subMenuPresenter;

    IFC_RETURN(m_wrSubPresenter.As(&subMenuPresenter));
    if (subMenuPresenter)
    {
        ctl::ComPtr<ISubMenuOwner> subMenuOwner;
        IFC_RETURN(subMenuPresenter->get_Owner(&subMenuOwner));

        if (subMenuOwner)
        {
            IFC_RETURN(subMenuOwner->CancelCloseSubMenu());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::GetParentMenuFlyoutSubItem(
    _In_ CDependencyObject* nativeDO,
    _Outptr_ CDependencyObject** ppMenuFlyoutSubItem)
{
    DXamlCore* pCore = DXamlCore::GetCurrent();
    ctl::ComPtr<DependencyObject> spTarget;
    IFC_RETURN(pCore->GetPeer(nativeDO, &spTarget));

    ctl::ComPtr<MenuFlyoutPresenter> spThis(spTarget.Cast<MenuFlyoutPresenter>());
    ctl::ComPtr<ISubMenuOwner> spResult;
    IFC_RETURN(spThis->get_Owner(&spResult));

    ctl::ComPtr<MenuFlyoutSubItem> spResultAsMenuFlyoutSubItem = spResult.AsOrNull<MenuFlyoutSubItem>();

    if (spResultAsMenuFlyoutSubItem)
    {
        *ppMenuFlyoutSubItem = static_cast<CDependencyObject*>(spResultAsMenuFlyoutSubItem.Cast<DependencyObject>()->GetHandle());
    }
    else
    {
        *ppMenuFlyoutSubItem = nullptr;
    }

    return S_OK;
}


_Check_return_ HRESULT
MenuFlyoutPresenter::UpdateTemplateSettings()
{
    ctl::ComPtr<IMenuFlyoutPresenterTemplateSettings> templateSettings;
    IFC_RETURN(get_TemplateSettings(&templateSettings));
    MenuFlyoutPresenterTemplateSettings* templateSettingsConcrete = templateSettings.Cast<MenuFlyoutPresenterTemplateSettings>();

    ctl::ComPtr<MenuFlyout> ownerFlyout;
    IFC_RETURN(GetParentMenuFlyout(&ownerFlyout));

    if (ownerFlyout && templateSettingsConcrete)
    {
        // Query MenuFlyout Content MinWidth, given the input mode, from resource dictionary.
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        IFC_RETURN(get_Resources(&resources));

        double flyoutContentMinWidth = 0.0;
        resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(
            (ownerFlyout->GetInputDeviceTypeUsedToOpen() == DirectUI::InputDeviceType::Touch || ownerFlyout->GetInputDeviceTypeUsedToOpen() == DirectUI::InputDeviceType::GamepadOrRemote)
                ? L"FlyoutThemeTouchMinWidth" : L"FlyoutThemeMinWidth").Get(), &flyoutContentMinWidth);

        wf::Rect visibleBounds = {};
        IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), &visibleBounds));

        IFC_RETURN(templateSettingsConcrete->put_FlyoutContentMinWidth(DoubleUtil::Min(visibleBounds.Width, flyoutContentMinWidth)));
    }

    double maxItemKeyboardAcceleratorTextWidth = 0;

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> menuItems;
    IFC_RETURN(get_Items(&menuItems));

    // MenuFlyoutItem's alignment changes based on the layout of other MenuFlyoutItems.
    // This check looks through all MenuFlyoutItems in our items source and finds the max
    // keyboard accelerator label width, which affects the look of other items.
    UINT nCount = 0;
    IFC_RETURN(menuItems.Cast<ItemCollection>()->get_Size(&nCount));
    for (UINT i = 0; i < nCount; ++i)
    {
        ctl::ComPtr<IMenuFlyoutItemBase> item;
        ctl::ComPtr<IMenuFlyoutItem> itemAsMenuItem;

        IFC_RETURN(menuItems.Cast<ItemCollection>()->GetAt(i, &item));

        itemAsMenuItem = item.AsOrNull<IMenuFlyoutItem>();
        if (itemAsMenuItem)
        {
            wf::Size desiredSize = {};
            double desiredWidth = 0;

            IFC_RETURN(itemAsMenuItem.Cast<MenuFlyoutItem>()->GetKeyboardAcceleratorTextDesiredSize(&desiredSize));
            desiredWidth = static_cast<double>(desiredSize.Width);

            if (desiredWidth > maxItemKeyboardAcceleratorTextWidth)
            {
                maxItemKeyboardAcceleratorTextWidth = desiredWidth;
            }
        }
    }

    for (UINT i = 0; i < nCount; ++i)
    {
        ctl::ComPtr<IMenuFlyoutItemBase> item;
        ctl::ComPtr<IMenuFlyoutItem> itemAsMenuItem;

        IFC_RETURN(menuItems.Cast<ItemCollection>()->GetAt(i, &item));

        itemAsMenuItem = item.AsOrNull<IMenuFlyoutItem>();
        if (itemAsMenuItem)
        {
            IFC_RETURN(itemAsMenuItem.Cast<MenuFlyoutItem>()->UpdateTemplateSettings(maxItemKeyboardAcceleratorTextWidth));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    // Sometimes GotFocus can come in before we're loaded, in which case focusing the first item may not have succeeded.
    // In that case, we should focus on load.
    IFC_RETURN(SetInitialFocus());
    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutPresenter::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    IFC_RETURN(MenuFlyoutPresenterGenerated::OnGotFocus(pArgs));
    IFC_RETURN(SetInitialFocus());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::SetInitialFocus()
{
    auto focusState = xaml::FocusState_Unfocused;
    IFC_RETURN(get_FocusState(&focusState));

    if (m_iFocusedIndex == -1 &&
        focusState != xaml::FocusState_Unfocused)
    {
        // The MenuFlyoutPresenter gets focused for the first time right after it is opened.
        // In this case we want to send focus to the first focusable item.
        IFC_RETURN(CycleFocus(TRUE /* shouldCycleDown */, focusState));
    }
    else if (focusState == xaml::FocusState_Unfocused)
    {
        // A child element got focus, so make sure we keep m_iFocusedIndex in sync
        // with it.
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());

        ctl::ComPtr<DependencyObject> focusedElement;

        if (CDependencyObject* coreFocusedElement = focusManager->GetFocusedElementNoRef())
        {
            IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(coreFocusedElement, &focusedElement));
        }

        // Since GotFocus is an async event, the focused element could be null if we got it
        // after the popup closes, which clears focus.
        if (focusedElement)
        {
            int focusedElementIndex = -1;
            IFC_RETURN(IndexFromContainer(focusedElement.Get(), &focusedElementIndex));
            if (focusedElementIndex != -1)
            {
                m_iFocusedIndex = focusedElementIndex;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutPresenter::EnsureInitialFocusIndex()
{
    if (m_iFocusedIndex == -1)
    {
        ctl::ComPtr<DependencyObject> focusedElement;

        IFC_RETURN(GetFocusedElement(&focusedElement));

        if (!ctl::are_equal(this, focusedElement.Get()))
        {
            UINT menuItemsCount = 0;
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> menuItems;

            IFC_RETURN(get_Items(&menuItems));
            IFC_RETURN(menuItems.Cast<ItemCollection>()->get_Size(&menuItemsCount));

            for (UINT i = 0; i < menuItemsCount; ++i)
            {
                ctl::ComPtr<IInspectable> itemAsIInspectable;
                ctl::ComPtr<MenuFlyoutItem> menuItem;

                IFC_RETURN(menuItems.Cast<ItemCollection>()->GetAt(i, &itemAsIInspectable));

                menuItem = itemAsIInspectable.AsOrNull<IMenuFlyoutItem>().Cast<MenuFlyoutItem>();

                if (ctl::are_equal(menuItem.Get(), focusedElement.Get()))
                {
                    m_iFocusedIndex = i;
                    break;
                }
            }

            ASSERT(m_iFocusedIndex != -1);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::GetPositionInSetHelper(const ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase>& item, _Out_ INT* returnValue)
{
    *returnValue = -1;

    ctl::ComPtr<MenuFlyoutPresenter> presenter;
    IFC_RETURN(item.Cast<MenuFlyoutItemBase>()->GetParentMenuFlyoutPresenter(&presenter));

    if (presenter)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
        IFC_RETURN(presenter->get_Items(&items));

        UINT indexOfItem = 0;
        BOOLEAN found = FALSE;
        IFC_RETURN(items.Cast<ItemCollection>()->IndexOf(item.Get(), &indexOfItem, &found));
        if (found)
        {
            // Iterate through the items preceding this item and subtract the number
            // of separators and collapsed items to get its position in the set.
            INT positionInSet = static_cast<INT>(indexOfItem);

            for (UINT i = 0; i < indexOfItem; ++i)
            {
                ctl::ComPtr<IInspectable> collectionItem;
                IFC_RETURN(items.Cast<ItemCollection>()->GetAt(i, &collectionItem));
                if (collectionItem)
                {
                    if (ctl::is<xaml_controls::IMenuFlyoutSeparator>(collectionItem))
                    {
                        --positionInSet;
                    }
                    else
                    {
                        ctl::ComPtr<xaml::IUIElement> itemAsUI = collectionItem.AsOrNull<xaml::IUIElement>();
                        if (itemAsUI)
                        {
                            auto visibility = xaml::Visibility_Collapsed;
                            IFC_RETURN(itemAsUI->get_Visibility(&visibility));

                            if (xaml::Visibility_Visible != visibility)
                            {
                                --positionInSet;
                            }
                        }
                    }
                }
            }

            ASSERT(positionInSet >= 0);

            *returnValue = positionInSet + 1;  // Add 1 to convert from a 0-based index to a 1-based index.
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::GetSizeOfSetHelper(const ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase>& item, _Out_ INT* returnValue)
{
    *returnValue = -1;

    ctl::ComPtr<MenuFlyoutPresenter> presenter;
    IFC_RETURN(item.Cast<MenuFlyoutItemBase>()->GetParentMenuFlyoutPresenter(&presenter));

    if (presenter)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
        IFC_RETURN(presenter->get_Items(&items));

        UINT itemsCount = 0;
        IFC_RETURN(items.Cast<ItemCollection>()->get_Size(&itemsCount));

        // Iterate through the parent presenters items and subtract the
        // number of separators and collapsed items from the total count
        // to get the size of the set.
        INT sizeOfSet = static_cast<INT>(itemsCount);

        for (UINT i = 0; i < itemsCount; ++i)
        {
            ctl::ComPtr<IInspectable> collectionItem;
            IFC_RETURN(items.Cast<ItemCollection>()->GetAt(i, &collectionItem));
            if (collectionItem)
            {
                if (ctl::is<xaml_controls::IMenuFlyoutSeparator>(collectionItem))
                {
                    --sizeOfSet;
                }
                else
                {
                    ctl::ComPtr<xaml::IUIElement> itemAsUI = collectionItem.AsOrNull<xaml::IUIElement>();
                    if (itemAsUI)
                    {
                        auto visibility = xaml::Visibility_Collapsed;
                        IFC_RETURN(itemAsUI->get_Visibility(&visibility));

                        if (xaml::Visibility_Visible != visibility)
                        {
                            --sizeOfSet;
                        }
                    }
                }
            }
        }

        ASSERT(sizeOfSet >= 0);

        *returnValue = sizeOfSet;
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::get_OwnerImpl(_Outptr_result_maybenull_ xaml_controls::ISubMenuOwner** ppValue)
{
    ctl::ComPtr<ISubMenuOwner> owner;
    owner = m_wrOwner.AsOrNull<ISubMenuOwner>();
    *ppValue = owner.Detach();

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::put_OwnerImpl(_In_opt_ xaml_controls::ISubMenuOwner* pValue)
{
    IFC_RETURN(ctl::AsWeak(pValue, &m_wrOwner));
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::get_OwningMenuImpl(_Outptr_result_maybenull_ xaml_controls::IMenu** ppValue)
{
    ctl::ComPtr<IMenu> owningMenu;
    owningMenu = m_wrOwningMenu.AsOrNull<IMenu>();
    *ppValue = owningMenu.Detach();

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::put_OwningMenuImpl(_In_opt_ xaml_controls::IMenu* pValue)
{
    IFC_RETURN(ctl::AsWeak(pValue, &m_wrOwningMenu));
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::get_SubPresenterImpl(_Outptr_result_maybenull_ xaml_controls::IMenuPresenter** ppValue)
{
    ctl::ComPtr<IMenuPresenter> subPresenter;
    subPresenter = m_wrSubPresenter.AsOrNull<IMenuPresenter>();
    *ppValue = subPresenter.Detach();

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutPresenter::put_SubPresenterImpl(_In_opt_ xaml_controls::IMenuPresenter* pValue)
{
    IFC_RETURN(ctl::AsWeak(pValue, &m_wrSubPresenter));
    return S_OK;
}

void MenuFlyoutPresenter::SetDepth(_In_ UINT depth)
{
    m_depth = depth;
}
UINT MenuFlyoutPresenter::GetDepth()
{
    return m_depth;
}