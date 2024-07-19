// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a MenuFlyoutSubItem.
//
//  MenuFlyoutSubItem is for supporting the cascade menu style to display
//  the sub menu item on the MenuFlyout. The MenuFlyoutSubItem will be shown
//  by the mouse over, pointer released or keyboard right key operation.
//  The displayed MenuFlyoutSubItems will be automatically light dismissed
//  whenever MenuFlyout is closed.

#include "precomp.h"
#include "MenuFlyoutSubItem.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuPopupThemeTransition.g.h"
#include "Popup.g.h"
#include "MenuFlyoutSubItemAutomationPeer.g.h"
#include "VisualTreeHelper.h"
#include "CascadingMenuHelper.h"
#include "ElevationHelper.h"
#include "XamlRoot.g.h"
#include "ElementSoundPlayerService_Partial.h"
#include "WrlHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment to output debugging information
// #define MFSI_DEBUG

MenuFlyoutSubItem::MenuFlyoutSubItem()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: MenuFlyoutSubItem.", this));
#endif // MFSI_DEBUG
}

_Check_return_ HRESULT
MenuFlyoutSubItem::PrepareState()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: PrepareState.", this));
#endif // MFSI_DEBUG

    ctl::ComPtr<MenuFlyoutItemBaseCollection> spItems;

    IFC_RETURN(MenuFlyoutSubItemGenerated::PrepareState());

    // Create the sub menu items collection and set the owner
    IFC_RETURN(ctl::make(&spItems));
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spItems->GetHandle()), GetHandle()));
    SetPtrValue(m_tpItems, spItems);
    IFC_RETURN(put_Items(spItems.Get()));

    ctl::ComPtr<CascadingMenuHelper> menuHelper;
    IFC_RETURN(ctl::make(this, &menuHelper));
    SetPtrValue(m_menuHelper, menuHelper.Get());

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::DisconnectFrameworkPeerCore()
{
    // Ensure the clean up the items whenever MenuFlyoutSubItem is disconnected
    if (m_tpItems.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpItems.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpItems.GetAsCoreDO()), nullptr));
    }

    IFC_RETURN(MenuFlyoutSubItemGenerated::DisconnectFrameworkPeerCore());

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutSubItem::OnApplyTemplate()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnApplyTemplate.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnApplyTemplate());
    IFC_RETURN(m_menuHelper->OnApplyTemplate());
    return S_OK;
}

// PointerEntered event handler that shows the MenuFlyoutSubItem
// whenever the pointer is over to the MenuFlyoutSubItem.
// In case of touch, the MenuFlyoutSubItem will be shown by
// PointerReleased event.
IFACEMETHODIMP
MenuFlyoutSubItem::OnPointerEntered(
_In_ xaml_input::IPointerRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnPointerEntered.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnPointerEntered(args));
    IFC_RETURN(UpdateParentOwner(nullptr /*parentMenuFlyoutPresenter*/));
    IFC_RETURN(m_menuHelper->OnPointerEntered(args));
    return S_OK;
}

// PointerExited event handler that ensures the close MenuFlyoutSubItem
// whenever the pointer over is out of the current MenuFlyoutSubItem or
// out of the main presenter. If the exited point is on MenuFlyoutSubItem
// or sub presenter position, we want to keep the opened MenuFlyoutSubItem.
IFACEMETHODIMP
MenuFlyoutSubItem::OnPointerExited(
_In_ xaml_input::IPointerRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnPointerExited.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnPointerExited(args));

    bool parentIsSubMenu = false;
    ctl::ComPtr<MenuFlyoutPresenter> parentPresenter;
    IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));

    if (parentPresenter)
    {
        parentIsSubMenu = parentPresenter->IsSubPresenter();
    }

    IFC_RETURN(m_menuHelper->OnPointerExited(args, parentIsSubMenu));
    return S_OK;
}

// PointerPressed event handler that ensures the pressed state.
IFACEMETHODIMP
MenuFlyoutSubItem::OnPointerPressed(
_In_ xaml_input::IPointerRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnPointerPressed.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnPointerPressed(args));
    IFC_RETURN(m_menuHelper->OnPointerPressed(args));
    return S_OK;
}

// PointerReleased event handler that shows MenuFlyoutSubItem in
// case of touch input.
IFACEMETHODIMP
MenuFlyoutSubItem::OnPointerReleased(
_In_ xaml_input::IPointerRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnPointerReleased.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnPointerReleased(args));
    IFC_RETURN(m_menuHelper->OnPointerReleased(args));
    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutSubItem::OnGotFocus(
_In_ xaml::IRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnGotFocus.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnGotFocus(args));
    IFC_RETURN(m_menuHelper->OnGotFocus(args));
    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutSubItem::OnLostFocus(
_In_ xaml::IRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnLostFocus.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnLostFocus(args));
    IFC_RETURN(m_menuHelper->OnLostFocus(args));
    return S_OK;
}

// KeyDown event handler that handles the keyboard navigation between
// the menu items and shows the MenuFlyoutSubItem in case of hitting
// the enter or right arrow key.
IFACEMETHODIMP
MenuFlyoutSubItem::OnKeyDown(
_In_ xaml_input::IKeyRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnKeyDown.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnKeyDown(args));

    BOOLEAN handled = false;
    IFC_RETURN(args->get_Handled(&handled));
    bool shouldHandleEvent = false;

    if (!handled)
    {
        ctl::ComPtr<MenuFlyoutPresenter> spParentPresenter;

        IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentPresenter));

        if (spParentPresenter)
        {
            auto key = wsy::VirtualKey_None;

            IFC_RETURN(args->get_Key(&key));

            // Navigate each item with the arrow down or up key
            if (key == wsy::VirtualKey_Down || key == wsy::VirtualKey_Up)
            {
                IFC_RETURN(spParentPresenter->HandleUpOrDownKey(key == wsy::VirtualKey_Down));
                IFC_RETURN(UpdateVisualState());

                // If we handle the event here, it won't get handled in m_menuHelper->OnKeyDown,
                // so we'll do that afterwards.
                shouldHandleEvent = true;
            }
        }
    }

    IFC_RETURN(m_menuHelper->OnKeyDown(args));
    IFC_RETURN(args->put_Handled(!!shouldHandleEvent));
    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutSubItem::OnKeyUp(
_In_ xaml_input::IKeyRoutedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnKeyUp.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnKeyUp(args));
    IFC_RETURN(m_menuHelper->OnKeyUp(args));
    return S_OK;
}

// Ensure the creating the popup and menu presenter to show the MenuFlyoutSubItem.
_Check_return_ HRESULT
MenuFlyoutSubItem::EnsurePopupAndPresenter()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: EnsurePopupAndPresenter.", this));
#endif // MFSI_DEBUG

    if (!m_tpPopup)
    {
        ctl::ComPtr<MenuFlyout> spParentMenuFlyout;
        ctl::ComPtr<MenuFlyoutPresenter> spParentMenuFlyoutPresenter;
        ctl::ComPtr<IControl> spPresenter;
        ctl::ComPtr<IUIElement> spPresenterAsUI;
        ctl::ComPtr<IFrameworkElement> spPresenterAsFE;
        ctl::ComPtr<Popup> spPopup;

        IFC_RETURN(ctl::make(&spPopup));
        IFC_RETURN(spPopup->put_IsSubMenu(TRUE));

        IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentMenuFlyoutPresenter));
        if (spParentMenuFlyoutPresenter)
        {
            IFC_RETURN(spParentMenuFlyoutPresenter->GetParentMenuFlyout(&spParentMenuFlyout));
            // Set the windowed Popup if the MenuFlyout is set the windowed Popup
            if (spParentMenuFlyout && spParentMenuFlyout->IsWindowedPopup())
            {
                IFC_RETURN(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->SetIsWindowed());

                // Ensure the sub menu is the windowed Popup
                ASSERT(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->IsWindowed());

                ctl::ComPtr<xaml::IXamlRoot> xamlRoot = XamlRoot::GetForElementStatic(spParentMenuFlyoutPresenter.Get());
                if (xamlRoot)
                {
                    IFC_RETURN(spPopup.Cast<Popup>()->put_XamlRoot(xamlRoot.Get()));
                }
            }
        }

        IFC_RETURN(CreateSubPresenter(&spPresenter));
        IFC_RETURN(spPresenter.As(&spPresenterAsUI));

        if (spParentMenuFlyoutPresenter)
        {
            UINT parentDepth = spParentMenuFlyoutPresenter->GetDepth();
            spPresenter.Cast<MenuFlyoutPresenter>()->SetDepth(parentDepth+1);
        }

        IFC_RETURN(spPopup->put_Child(spPresenterAsUI.Get()));

        IFC_RETURN(SetPtrValueWithQI(m_tpPresenter, spPresenter.Get()));
        IFC_RETURN(SetPtrValueWithQI(m_tpPopup, spPopup.Get()));

        IFC_RETURN(static_cast<ItemsControl*>(m_tpPresenter.Get())->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

        IFC_RETURN(spPresenter.As(&spPresenterAsFE));
        IFC_RETURN(m_epPresenterSizeChangedHandler.AttachEventHandler(
            spPresenterAsFE.Get(),
            [this](IInspectable* pSender, xaml::ISizeChangedEventArgs* args)
        {
            return OnPresenterSizeChanged(pSender, args);
        }));

        IFC_RETURN(m_menuHelper->SetSubMenuPresenter(spPresenter.Cast<Control>()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::ForwardPresenterProperties(
    _In_ MenuFlyout* pOwnerMenuFlyout,
    _In_ MenuFlyoutPresenter* pParentMenuFlyoutPresenter,
    _In_ MenuFlyoutPresenter* pSubMenuFlyoutPresenter)
{
    ctl::ComPtr<IStyle> spStyle;
    xaml::ElementTheme parentPresenterTheme;
    ctl::ComPtr<IInspectable> spDataContext;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    wrl_wrappers::HString strLanguage;
    BOOLEAN isTextScaleFactorEnabled = TRUE;
    ctl::ComPtr<IFrameworkElement> spPopupAsFE;
    ctl::ComPtr<MenuFlyoutPresenter> spSubMenuFlyoutPresenter(pSubMenuFlyoutPresenter);
    ctl::ComPtr<IControl> spSubMenuFlyoutPresenterAsControl;
    ctl::ComPtr<xaml::IDependencyObject> spThisAsDO = this;

    ASSERT(pOwnerMenuFlyout && pParentMenuFlyoutPresenter && pSubMenuFlyoutPresenter);

    IFC_RETURN(spSubMenuFlyoutPresenter.As(&spSubMenuFlyoutPresenterAsControl));

    // Set the sub presenter style from the MenuFlyout's presenter style
    IFC_RETURN(pOwnerMenuFlyout->get_MenuFlyoutPresenterStyle(&spStyle));

    if (spStyle.Get())
    {
        IFC_RETURN(static_cast<Control*>(pSubMenuFlyoutPresenter)->put_Style(spStyle.Get()));
    }
    else
    {
        IFC_RETURN(static_cast<Control*>(pSubMenuFlyoutPresenter)->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
    }

    // Set the sub presenter's RequestTheme from the parent presenter's RequestTheme
    IFC_RETURN(pParentMenuFlyoutPresenter->get_RequestedTheme(&parentPresenterTheme));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_RequestedTheme(parentPresenterTheme));

    // Set the sub presenter's DataContext from the parent presenter's DataContext
    IFC_RETURN(pParentMenuFlyoutPresenter->get_DataContext(&spDataContext));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_DataContext(spDataContext.Get()));

    // Set the sub presenter's FlowDirection from the current sub menu item's FlowDirection
    IFC_RETURN(get_FlowDirection(&flowDirection));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_FlowDirection(flowDirection));

    // Set the popup's FlowDirection from the current FlowDirection
    IFC_RETURN(m_tpPopup.As(&spPopupAsFE));
    IFC_RETURN(spPopupAsFE->put_FlowDirection(flowDirection));
    // Also set the popup's theme. If there is a SystemBackdrop on the menu, it'll be watching the theme on the popup
    // itself rather than the presenter set as the popup's child.
    IFC_RETURN(spPopupAsFE->put_RequestedTheme(parentPresenterTheme));

    // Set the sub presenter's Language from the parent presenter's Language
    IFC_RETURN(pParentMenuFlyoutPresenter->get_Language(strLanguage.GetAddressOf()));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_Language(strLanguage.Get()));

    // Set the sub presenter's IsTextScaleFactorEnabledInternal from the parent presenter's IsTextScaleFactorEnabledInternal
    IFC_RETURN(pParentMenuFlyoutPresenter->get_IsTextScaleFactorEnabledInternal(&isTextScaleFactorEnabled));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_IsTextScaleFactorEnabledInternal(isTextScaleFactorEnabled));

    xaml::ElementSoundMode soundMode = DirectUI::ElementSoundPlayerService::GetEffectiveSoundMode(spThisAsDO.Cast<DependencyObject>());

    IFC_RETURN(spSubMenuFlyoutPresenterAsControl.Cast<Control>()->put_ElementSoundMode(soundMode));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutSubItem::ForwardSystemBackdropToPopup(_In_ MenuFlyout* pOwnerMenuFlyout)
{
    // Set the popup's SystemBackdrop from the parent flyout's SystemBackdrop. Note that the top-level menu is a
    // MenuFlyout with a SystemBackdrop property, but submenus are MenyFlyoutSubItems with no SystemBackdrop property,
    // so we just use the one set on the top-level menu. SystemBackdrop can handle having multiple parents, and will
    // keep a separate controller/configuration object per place it's used.
    ctl::ComPtr<ISystemBackdrop> flyoutSystemBackdrop = pOwnerMenuFlyout->GetSystemBackdrop();
    ctl::ComPtr<ISystemBackdrop> popupSystemBackdrop;
    IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->get_SystemBackdrop(&popupSystemBackdrop));
    if (flyoutSystemBackdrop.Get() != popupSystemBackdrop.Get())
    {
        IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->put_SystemBackdrop(flyoutSystemBackdrop.Get()));
    }

    return S_OK;
}

// Ensure that any currently open MenuFlyoutSubItems are closed
_Check_return_ HRESULT
MenuFlyoutSubItem::EnsureCloseExistingSubItems()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: EnsureCloseExistingSubItems.", this));
#endif // MFSI_DEBUG

    ctl::ComPtr<MenuFlyoutPresenter> spParentPresenter;

    IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentPresenter));
    if (spParentPresenter)
    {
        ctl::ComPtr<IMenuPresenter> openedSubPresenter;

        IFC_RETURN(spParentPresenter->get_SubPresenter(&openedSubPresenter));
        if (openedSubPresenter)
        {
            ctl::ComPtr<ISubMenuOwner> subMenuOwner;

            IFC_RETURN(openedSubPresenter->get_Owner(&subMenuOwner));
            if (subMenuOwner && !ctl::are_equal(subMenuOwner.Get(), this))
            {
                IFC_RETURN(openedSubPresenter->CloseSubMenu());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::get_IsOpen(_Out_ BOOLEAN *pIsOpen)
{
    *pIsOpen = FALSE;
    if (m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(pIsOpen))
    }
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::CreateSubPresenter(
_Outptr_ IControl** ppReturnValue)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: CreateSubPresenter.", this));
#endif // MFSI_DEBUG

    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;

    *ppReturnValue = nullptr;

    IFC_RETURN(ctl::make(&spPresenter));

    // Specify the sub MenuFlyoutPresenter
    spPresenter.Cast<MenuFlyoutPresenter>()->m_isSubPresenter = true;
    IFC_RETURN(spPresenter.Cast<MenuFlyoutPresenter>()->put_Owner(this));

    IFC_RETURN(spPresenter.MoveTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::UpdateParentOwner(
    _In_opt_ MenuFlyoutPresenter* parentMenuFlyoutPresenter)
{
    ctl::ComPtr<MenuFlyoutPresenter> parentPresenter(parentMenuFlyoutPresenter);
    if (!parentPresenter)
    {
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));
    }
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: UpdateParentOwner - parentPresenter=0x%p.", this, parentPresenter));
#endif // MFSI_DEBUG

    if (parentPresenter)
    {
        ctl::ComPtr<ISubMenuOwner> parentSubMenuOwner;
        IFC_RETURN(parentPresenter.Cast<MenuFlyoutPresenter>()->get_Owner(&parentSubMenuOwner));

#ifdef MFSI_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: UpdateParentOwner - parentSubMenuOwner=0x%p.", this, parentSubMenuOwner));
#endif // MFSI_DEBUG

        if (parentSubMenuOwner)
        {
            IFC_RETURN(put_ParentOwner(parentSubMenuOwner.Get()));
        }
    }

    return S_OK;
}

// Set the popup open or close status for MenuFlyoutSubItem and ensure the
// focus to the current presenter.
_Check_return_ HRESULT
MenuFlyoutSubItem::SetIsOpen(
_In_ BOOLEAN isOpen)
{
    BOOLEAN isOpened = FALSE;
    BOOLEAN focusUpdated = FALSE;

    IFC_RETURN(m_tpPopup->get_IsOpen(&isOpened))

#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: SetIsOpen isOpen=%d, isOpened=%d.", this, isOpen, isOpened));
#endif // MFSI_DEBUG

    if (isOpen != isOpened)
    {
        IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_Owner(isOpen ? this : nullptr));

        ctl::ComPtr<MenuFlyoutPresenter> parentPresenter;
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));

        if (parentPresenter)
        {
            IFC_RETURN(parentPresenter.Cast<MenuFlyoutPresenter>()->put_SubPresenter(isOpen ? m_tpPresenter.Cast<MenuFlyoutPresenter>() : nullptr));

            ctl::ComPtr<IMenu> owningMenu;
            IFC_RETURN(parentPresenter->get_OwningMenu(&owningMenu));

            if (owningMenu)
            {
                IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_OwningMenu(isOpen ? owningMenu.Get() : nullptr));
            }

            IFC_RETURN(UpdateParentOwner(parentPresenter.Get()));
        }

        VisualTree* visualTree = VisualTree::GetForElementNoRef(GetHandle());
        if (visualTree)
        {
            // Put the popup on the same VisualTree as this flyout sub item to make sure it shows up in the right place
            static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())->SetAssociatedVisualTree(visualTree);
        }

        // Set the popup open or close state
        IFC_RETURN(m_tpPopup->put_IsOpen(isOpen));

        // Set the focus to the displayed sub menu presenter when MenuFlyoutSubItem is opened and
        // set the focus back to the original sub item when the displayed sub menu presenter is closed.
        if (isOpen)
        {
            ctl::ComPtr<IDependencyObject> spPresenterAsDO;

            IFC_RETURN(m_tpPresenter.As(&spPresenterAsDO));

            // Set the focus to the displayed sub menu presenter to navigate the each sub items
            IFC_RETURN(DependencyObject::SetFocusedElement(
                spPresenterAsDO.Cast<DependencyObject>(),
                xaml::FocusState_Programmatic,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }
        else
        {
            ctl::ComPtr<xaml::IDependencyObject> spThisAsDO = this;

            // Set the focus to the sub menu item
            IFC_RETURN(DependencyObject::SetFocusedElement(
                spThisAsDO.Cast<DependencyObject>(),
                xaml::FocusState_Programmatic,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }

        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::Open()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: Open.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->OpenSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::Close()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: Close.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->CloseSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::ChangeVisualState(
    _In_ bool bUseTransitions)
{
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIgnored = FALSE;
    bool hasToggleMenuItem = false;
    bool hasIconMenuItem = false;
    BOOLEAN bIsPopupOpened = FALSE;
    BOOLEAN bIsDelayCloseTimerRunning = FALSE;
    bool showSubMenuOpenedState = false;
    bool shouldBeNarrow = false;
    auto focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    IFC_RETURN(get_FocusState(&focusState));
    IFC_RETURN(GetShouldBeNarrow(&shouldBeNarrow));

    IFC_RETURN(GetParentMenuFlyoutPresenter(&spPresenter));
    if (spPresenter)
    {
        hasToggleMenuItem = spPresenter->GetContainsToggleItems();
        hasIconMenuItem = spPresenter->GetContainsIconItems();
    }

    if(m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(&bIsPopupOpened));
    }

    IFC_RETURN(m_menuHelper->IsDelayCloseTimerRunning(&bIsDelayCloseTimerRunning));
    if(bIsPopupOpened && !bIsDelayCloseTimerRunning)
    {
        showSubMenuOpenedState = true;
    }

    // CommonStates
    if (!bIsEnabled)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (showSubMenuOpenedState)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"SubMenuOpened", &bIgnored));
    }
    else if (m_menuHelper->IsPressed())
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (m_menuHelper->IsPointerOver())
    {
        IFC_RETURN(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    // FocusStates
    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState)
        {
            IFC_RETURN(GoToState(bUseTransitions, L"PointerFocused", &bIgnored));
        }
        else
        {
            IFC_RETURN(GoToState(bUseTransitions, L"Focused", &bIgnored));
        }
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }

    // CheckPlaceholderStates
    if (hasToggleMenuItem && hasIconMenuItem)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"CheckAndIconPlaceholder", &bIgnored));
    }
    else if (hasToggleMenuItem)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"CheckPlaceholder", &bIgnored));
    }
    else if (hasIconMenuItem)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"IconPlaceholder", &bIgnored));
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"NoPlaceholder", &bIgnored));
    }

    // PaddingSizeStates
    if (shouldBeNarrow)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"NarrowPadding", &bIgnored));
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"DefaultPadding", &bIgnored));
    }

    return S_OK;
}

// MenuFlyoutSubItem's presenter size changed event handler that
// adjust the sub presenter position to the proper space area
// on the available window rect.
_Check_return_ HRESULT
MenuFlyoutSubItem::OnPresenterSizeChanged(
_In_ IInspectable* pSender,
_In_ xaml::ISizeChangedEventArgs* args) noexcept
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnPresenterSizeChanged.", this));
#endif // MFSI_DEBUG

    // Creating MenuPopupThemeTransition before CascadingMenuHelper::OnPresenterSizeChanged call so that its MenuFlyoutSubItem::SetSubMenuDirection call has an effect.

    if (!m_tpMenuPopupThemeTransition)
    {
        ctl::ComPtr<MenuFlyoutPresenter> parentMenuFlyoutPresenter;
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentMenuFlyoutPresenter));

        // Get how many sub menus deep we are. We need this number to know what kind of Z
        // offset to use for displaying elevation. The menus aren't parented in the visual
        // hierarchy so that has to be applied with an additional transform.
        UINT depth = 1;
        if (parentMenuFlyoutPresenter)
        {
            depth = parentMenuFlyoutPresenter->GetDepth() + 1;
        }

        ctl::ComPtr<xaml_animation::ITransition> spMenuPopupChildTransition;
        IFC_RETURN(MenuFlyout::PreparePopupThemeTransitionsAndShadows(static_cast<Popup*>(m_tpPopup.Get()), 0.67 /* closedRatioConstant */, depth, &spMenuPopupChildTransition));
        IFC_RETURN(spMenuPopupChildTransition.Cast<MenuPopupThemeTransition>()->put_Direction(xaml_primitives::AnimationDirection_Top));
        SetPtrValue(m_tpMenuPopupThemeTransition, spMenuPopupChildTransition.Get());
    }

    IFC_RETURN(m_menuHelper->OnPresenterSizeChanged(pSender, args, m_tpPopup.Cast<Popup>()));

    // Update the OpenedLength property of the ThemeTransition.
    double openedLength = 0.0;
    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_ActualHeight(&openedLength));
    IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_OpenedLength(openedLength));

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::ClearStateFlags()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: ClearStateFlags.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->ClearStateFlags());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::OnIsEnabledChanged(
_In_ IsEnabledChangedEventArgs* args)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnIsEnabledChanged.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(MenuFlyoutSubItemGenerated::OnIsEnabledChanged(args));
    IFC_RETURN(m_menuHelper->OnIsEnabledChanged(args));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::OnVisibilityChanged()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OnVisibilityChanged.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->OnVisibilityChanged());
    return S_OK;
}

IFACEMETHODIMP MenuFlyoutSubItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    *ppAutomationPeer = nullptr;
    ctl::ComPtr<MenuFlyoutSubItemAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::MenuFlyoutSubItemAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::GetPlainText(_Out_ HSTRING* strPlainText)
{
    wrl_wrappers::HString strText;
    IFC_RETURN(get_Text(strText.ReleaseAndGetAddressOf()));
    IFC_RETURN(strText.CopyTo(strPlainText));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::get_IsSubMenuOpenImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(get_IsOpen(pValue));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::get_ParentOwnerImpl(_Outptr_ ISubMenuOwner** ppValue)
{
    ctl::ComPtr<ISubMenuOwner> parentOwner;
    parentOwner = m_wrParentOwner.AsOrNull<ISubMenuOwner>();
    *ppValue = parentOwner.Detach();

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::put_ParentOwnerImpl(_In_ ISubMenuOwner* pValue)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: put_ParentOwner - parentOwner=0x%p.", this, pValue));
#endif // MFSI_DEBUG

    IFC_RETURN(ctl::AsWeak(pValue, &m_wrParentOwner));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::SetSubMenuDirectionImpl(BOOLEAN isSubMenuDirectionUp)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: SetSubMenuDirection - MenuPopupThemeTransition exists=%d, isSubMenuDirectionUp=%d.", this, m_tpMenuPopupThemeTransition != nullptr, isSubMenuDirectionUp));
#endif // MFSI_DEBUG

    if (m_tpMenuPopupThemeTransition)
    {
        IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_Direction(
            isSubMenuDirectionUp ?
            xaml_primitives::AnimationDirection_Bottom :
            xaml_primitives::AnimationDirection_Top));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::PrepareSubMenuImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: PrepareSubMenu.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(EnsurePopupAndPresenter());

    ASSERT(m_tpPopup);
    ASSERT(m_tpPresenter);

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::OpenSubMenuImpl(wf::Point position)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: OpenSubMenu - position=(%f, %f).", this, position.X, position.Y));
#endif // MFSI_DEBUG

    IFC_RETURN(EnsurePopupAndPresenter());
    IFC_RETURN(EnsureCloseExistingSubItems());

    ctl::ComPtr<MenuFlyoutPresenter> parentMenuFlyoutPresenter;
    IFC_RETURN(GetParentMenuFlyoutPresenter(&parentMenuFlyoutPresenter));

    ctl::ComPtr<MenuFlyout> parentMenuFlyout;
    if (parentMenuFlyoutPresenter)
    {
        ctl::ComPtr<IMenu> owningMenu;
        IFC_RETURN(parentMenuFlyoutPresenter->get_OwningMenu(&owningMenu));
        IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_OwningMenu(owningMenu.Get()));

        IFC_RETURN(parentMenuFlyoutPresenter->GetParentMenuFlyout(&parentMenuFlyout));

        if (parentMenuFlyout)
        {
            // Update the TemplateSettings before it is opened.
            IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->SetParentMenuFlyout(parentMenuFlyout.Get()));
            IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->UpdateTemplateSettings());

            // Forward the parent presenter's properties to the sub presenter
            IFC_RETURN(ForwardPresenterProperties(
                parentMenuFlyout.Get(),
                parentMenuFlyoutPresenter.Get(),
                m_tpPresenter.Cast<MenuFlyoutPresenter>()));
        }
    }

    IFC_RETURN(m_tpPopup->put_HorizontalOffset(position.X));
    IFC_RETURN(m_tpPopup->put_VerticalOffset(position.Y));
    IFC_RETURN(SetIsOpen(TRUE));

    if (parentMenuFlyout)
    {
        // Note: This is the call that propagates the SystemBackdrop object set on either this FlyoutBase or its
        // MenuFlyoutPresenter to the Popup. A convenient place to do this is in ForwardTargetPropertiesToPresenter, but we
        // see cases where the MenuFlyoutPresenter's SystemBackdrop property is null until it enters the tree via the
        // Popup::Open call above. So trying to propagate it before opening the popup actually finds no SystemBackdrop, and
        // the popup is left with a transparent background. Do the propagation after the popup opens instead. Windowed
        // popups support having a backdrop set after the popup is open.
        IFC_RETURN(ForwardSystemBackdropToPopup(parentMenuFlyout.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::PositionSubMenuImpl(wf::Point position)
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: PositionSubMenu - (%f, %f).", this, position.X, position.Y));
#endif // MFSI_DEBUG

    if (position.X != -std::numeric_limits<float>::infinity())
    {
        IFC_RETURN(m_tpPopup->put_HorizontalOffset(position.X));
    }

    if (position.Y != -std::numeric_limits<float>::infinity())
    {
        IFC_RETURN(m_tpPopup->put_VerticalOffset(position.Y));
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::ClosePeerSubMenusImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: ClosePeerSubMenus.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(EnsureCloseExistingSubItems());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::CloseSubMenuImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: CloseSubMenu.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(SetIsOpen(FALSE));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::CloseSubMenuTreeImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: CloseSubMenuTree.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->CloseChildSubMenus());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::DelayCloseSubMenuImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: DelayCloseSubMenu.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->DelayCloseSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::CancelCloseSubMenuImpl()
{
#ifdef MFSI_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MFSI[0x%p]: CancelCloseSubMenu.", this));
#endif // MFSI_DEBUG

    IFC_RETURN(m_menuHelper->CancelCloseSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::RaiseAutomationPeerExpandCollapseImpl(
    _In_ BOOLEAN isOpen)
{
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN isListener = FALSE;

    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &isListener));
    if (isListener)
    {
        IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
        if (spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer.Cast<MenuFlyoutSubItemAutomationPeer>()->RaiseExpandCollapseAutomationEvent(isOpen));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::QueueRefreshItemsSource()
{
    // The items source might change multiple times in a single tick, so we'll coalesce the refresh
    // into a single event once all of the changes have completed.
    if (m_tpPresenter && !m_itemsSourceRefreshPending)
    {
        ctl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
        ctl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            dispatcherQueueStatics.ReleaseAndGetAddressOf()));

        IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(&dispatcherQueue));

        ctl::WeakRefPtr wrThis;
        boolean enqueued = false;

        IFC_RETURN(ctl::ComPtr<IMenuFlyoutSubItem>(this).AsWeak(&wrThis));

        IFC_RETURN(dispatcherQueue->TryEnqueue(
            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wrThis]() mutable {
                ctl::ComPtr<IMenuFlyoutSubItem> thisMenuFlyoutSubItem;
                IFC_RETURN(wrThis.As(&thisMenuFlyoutSubItem));

                if (thisMenuFlyoutSubItem)
                {
                    thisMenuFlyoutSubItem.Cast<MenuFlyoutSubItem>()->RefreshItemsSource();
                }

                return S_OK;
            }).Get(),
            &enqueued));

        IFCEXPECT_RETURN(enqueued);
        m_itemsSourceRefreshPending = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutSubItem::RefreshItemsSource()
{
    m_itemsSourceRefreshPending = false;

    ASSERT(m_tpPresenter);

    // Setting the items source to null and then back to Items causes the presenter to pick up any changes.
    IFC_RETURN(m_tpPresenter.Cast<ItemsControl>()->put_ItemsSource(nullptr));
    IFC_RETURN(m_tpPresenter.Cast<ItemsControl>()->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

    return S_OK;
}