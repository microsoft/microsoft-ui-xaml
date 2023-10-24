// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CascadingMenuHelper.h"
#include "AutomationPeer.g.h"
#include "Control.g.h"
#include "DispatcherTimer.g.h"
#include "FlyoutBase.g.h"
#include "Popup.g.h"
#include "ToolTipService.g.h"
#include "VisualTreeHelper.h"
#include "ElementSoundPlayerService_Partial.h"
#include "MenuFlyoutSubItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment to output debugging information
// #define CMH_DEBUG

CascadingMenuHelper::CascadingMenuHelper()
    : m_isPointerOver(false)
    , m_isPressed(false)
    , m_subMenuShowDelay(-1)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: CascadingMenuHelper.", this));
#endif // CMH_DEBUG
}

CascadingMenuHelper::~CascadingMenuHelper()
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: ~CascadingMenuHelper.", this));
#endif // CMH_DEBUG

    auto delayOpenMenuTimer = m_delayOpenMenuTimer.GetSafeReference();
    if (delayOpenMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: ~CascadingMenuHelper - Stopping m_delayOpenMenuTimer.", this));
#endif // CMH_DEBUG

        IGNOREHR(delayOpenMenuTimer->Stop());
    }

    auto delayCloseMenuTimer = m_delayCloseMenuTimer.GetSafeReference();
    if (delayCloseMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: ~CascadingMenuHelper - Stopping m_delayCloseMenuTimer.", this));
#endif // CMH_DEBUG
        IGNOREHR(delayCloseMenuTimer->Stop());
    }
}

_Check_return_ HRESULT CascadingMenuHelper::Initialize(_In_ IFrameworkElement* owner)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: Initialize.", this));
#endif // CMH_DEBUG

    ctl::ComPtr<IFrameworkElement> ownerLocal = owner;
    IFC_RETURN(ownerLocal.AsWeak(&m_wpOwner));

    IFC_RETURN(m_loadedHandler.AttachEventHandler(
        ownerLocal.Get(),
        [this](IInspectable *pSender, IRoutedEventArgs *args)
    {
        return ClearStateFlags();
    }));

    // Try and read from Reg Key
    HKEY key = nullptr;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\", 0, KEY_QUERY_VALUE, &key))
    {
        WCHAR Buffer[32] = { 0 };
        DWORD BufferSize = sizeof(Buffer);
        DWORD dwOutType;

        if (RegQueryValueEx(key, L"MenuShowDelay", NULL, &dwOutType/*REG_SZ*/, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
        {
            m_subMenuShowDelay = _wtoi(Buffer);
        }
        RegCloseKey(key);
    }

    // If the field wasn't successfully populated from the reg key
    // Or if the reg key contained a negative number
    // Then use a default value
    if (m_subMenuShowDelay < 0)
    {
        m_subMenuShowDelay = DefaultMenuShowDelay;
    }

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: Initialize - m_subMenuShowDelay=%d.", this, m_subMenuShowDelay));
#endif // CMH_DEBUG


    // Cascading menu owners should be access key scopes by default.
    IFC_RETURN(ownerLocal.Cast<FrameworkElement>()->put_IsAccessKeyScope(TRUE));

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnApplyTemplate()
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnApplyTemplate.", this));
#endif // CMH_DEBUG

    IFC_RETURN(UpdateOwnerVisualState());
    return S_OK;
}

// PointerEntered event handler that shows the sub menu
// whenever the pointer is over the sub menu owner.
// In case of touch, the sub menu item will be shown by
// PointerReleased event.
_Check_return_ HRESULT CascadingMenuHelper::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    BOOLEAN handled = FALSE;

    m_isPointerOver = true;

    IFC_RETURN(args->get_Handled(&handled));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerEntered - handled=%d.", this, handled));
#endif // CMH_DEBUG

    if (!handled)
    {
        ctl::ComPtr<ISubMenuOwner> owner;
        IFC_RETURN(m_wpOwner.As(&owner));

#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerEntered - owner=0x%p.", this, owner));
#endif // CMH_DEBUG

        if (owner)
        {
            ctl::ComPtr<ISubMenuOwner> parentOwner;
            IFC_RETURN(owner->get_ParentOwner(&parentOwner));

#ifdef CMH_DEBUG
            IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerEntered - parentOwner=0x%p.", this, parentOwner));
#endif // CMH_DEBUG

            if (parentOwner)
            {
                IFC_RETURN(parentOwner->CancelCloseSubMenu());
            }
        }

        ctl::ComPtr<xaml_input::IPointer> pointer;
        auto pointerDeviceType = mui::PointerDeviceType_Touch;

        IFC_RETURN(args->get_Pointer(&pointer));
        IFC_RETURN(pointer->get_PointerDeviceType(&pointerDeviceType));

#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerEntered - pointerDeviceType=%d.", this, pointerDeviceType));
#endif // CMH_DEBUG

        if (pointerDeviceType != mui::PointerDeviceType_Touch)
        {
            IFC_RETURN(CancelCloseSubMenu());

            IFC_RETURN(EnsureDelayOpenMenuTimer());

#ifdef CMH_DEBUG
            IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerEntered - Starting m_delayOpenMenuTimer.", this));
#endif // CMH_DEBUG

            IFC_RETURN(m_delayOpenMenuTimer->Start());

            IFC_RETURN(UpdateOwnerVisualState());
        }

        IFC_RETURN(args->put_Handled(TRUE));
    }

    return S_OK;
}

// PointerExited event handler that ensures we close the sub menu
// whenever the pointer leaves the current sub menu or
// the main presenter. If the exited point is on the sub menu owner
// or the sub menu, we want to keep the sub menu open.
_Check_return_ HRESULT CascadingMenuHelper::OnPointerExited(
    _In_ xaml_input::IPointerRoutedEventArgs* args,
    bool parentIsSubMenu)
{
    BOOLEAN handled = FALSE;

    m_isPointerOver = false;
    m_isPressed = false;

    IFC_RETURN(args->get_Handled(&handled));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerExited - handled=%d, parentIsSubMenu=%d.", this, handled, parentIsSubMenu));
#endif // CMH_DEBUG

    if (m_delayOpenMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerExited - Stopping m_delayOpenMenuTimer.", this));
#endif // CMH_DEBUG

        IFC_RETURN(m_delayOpenMenuTimer->Stop());
    }

    ctl::ComPtr<IUIElement> ownerAsUIE;
    IFC_RETURN(m_wpOwner.As(&ownerAsUIE));

    if (!handled && ownerAsUIE && ownerAsUIE.Cast<UIElement>()->IsInLiveTree())
    {
        ctl::ComPtr<xaml_input::IPointer> pointer;
        auto pointerDeviceType = mui::PointerDeviceType_Touch;

        IFC_RETURN(args->get_Pointer(&pointer));
        IFC_RETURN(pointer->get_PointerDeviceType(&pointerDeviceType));

#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerExited - pointerDeviceType=%d.", this, pointerDeviceType));
#endif // CMH_DEBUG

        if (mui::PointerDeviceType_Mouse == pointerDeviceType && !parentIsSubMenu)
        {
            ctl::ComPtr<IUIElement> subMenuPresenterAsUIE;
            IFC_RETURN(m_wpSubMenuPresenter.As(&subMenuPresenterAsUIE));

            if (subMenuPresenterAsUIE && subMenuPresenterAsUIE.Cast<UIElement>()->IsInLiveTree())
            {
                bool isOwnerOrSubMenuHit = false;
                BOOLEAN hasCurrent = FALSE;
                wf::Point point = {};
                ctl::ComPtr<ixp::IPointerPoint> pointerPoint;
                ctl::ComPtr<wfc::IIterable<xaml::UIElement*>> elements;
                ctl::ComPtr<wfc::IIterator<xaml::UIElement*>> iterator;

                IFC_RETURN(args->GetCurrentPoint(NULL, &pointerPoint));
                IFC_RETURN(pointerPoint->get_Position(&point));

                IFC_RETURN(VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(point, ownerAsUIE.Get(), TRUE /* includeAllElements */, &elements));
                IFC_RETURN(elements->First(&iterator));
                IFC_RETURN(iterator->get_HasCurrent(&hasCurrent));

                while (hasCurrent)
                {
                    ctl::ComPtr<xaml::IUIElement> element;

                    IFC_RETURN(iterator->get_Current(&element));

                    if (ownerAsUIE.Get() == element.Get() || subMenuPresenterAsUIE.Get() == element.Get())
                    {
                        isOwnerOrSubMenuHit = true;
                        break;
                    }

                    IFC_RETURN(iterator->MoveNext(&hasCurrent));
                }

                if (!isOwnerOrSubMenuHit)
                {
                    IFC_RETURN(VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(point, subMenuPresenterAsUIE.Get(), TRUE /* includeAllElements */, &elements));
                    IFC_RETURN(elements->First(&iterator));
                    IFC_RETURN(iterator->get_HasCurrent(&hasCurrent));

                    while (hasCurrent)
                    {
                        ctl::ComPtr<xaml::IUIElement> element;

                        IFC_RETURN(iterator->get_Current(&element));

                        if (ownerAsUIE.Get() == element.Get() || subMenuPresenterAsUIE.Get() == element.Get())
                        {
                            isOwnerOrSubMenuHit = true;
                            break;
                        }

                        IFC_RETURN(iterator->MoveNext(&hasCurrent));
                    }
                }

                // To close the sub menu, the pointer must be outside of the opened chain of sub-menus.
                if (!isOwnerOrSubMenuHit)
                {
                    IFC_RETURN(DelayCloseSubMenu());
                    IFC_RETURN(args->put_Handled(TRUE));
                }
            }
        }

        IFC_RETURN(UpdateOwnerVisualState());
    }

    return S_OK;
}

// PointerPressed event handler ensures that we're in the pressed state.
_Check_return_ HRESULT CascadingMenuHelper::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerPressed.", this));
#endif // CMH_DEBUG

    m_isPressed = true;

    IFC_RETURN(args->put_Handled(TRUE));

    return S_OK;
}

// PointerReleased event handler shows the sub menu in the
// case of touch input.
_Check_return_ HRESULT CascadingMenuHelper::OnPointerReleased(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    ctl::ComPtr<xaml_input::IPointer> pointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    m_isPressed = false;

    IFC_RETURN(args->get_Pointer(&pointer));
    IFC_RETURN(pointer->get_PointerDeviceType(&pointerDeviceType));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPointerReleased - pointerDeviceType=%d.", this, pointerDeviceType));
#endif // CMH_DEBUG

    // Show the sub menu in the case of touch and pen input.
    // In case of the mouse device, the sub menu will be shown whenever the pointer is over the sub menu owner.
    // Note that sub menu is also shown OnPointerOver with pen device.
    if (mui::PointerDeviceType_Touch == pointerDeviceType || mui::PointerDeviceType_Pen == pointerDeviceType)
    {
        IFC_RETURN(OpenSubMenu());
    }

    IFC_RETURN(args->put_Handled(TRUE));

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnGotFocus(_In_ xaml::IRoutedEventArgs* args)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnGotFocus.", this));
#endif // CMH_DEBUG

    IFC_RETURN(UpdateOwnerVisualState());
    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnLostFocus(_In_ xaml::IRoutedEventArgs* args)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnLostFocus.", this));
#endif // CMH_DEBUG

    m_isPressed = false;
    IFC_RETURN(UpdateOwnerVisualState());
    return S_OK;
}

// KeyDown event handler that handles the keyboard navigation between
// the menu items and shows the sub menu in the case where we hit
// the enter, space, or right arrow keys.
_Check_return_ HRESULT CascadingMenuHelper::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    BOOLEAN handled = FALSE;

    IFC_RETURN(args->get_Handled(&handled));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnKeyDown - handled=%d.", this, handled));
#endif // CMH_DEBUG

    if (!handled)
    {
        auto key = wsy::VirtualKey_None;
        IFC_RETURN(args->get_Key(&key));

        // Show the sub menu with the enter, space, or right arrow keys
        if (key == wsy::VirtualKey_Enter ||
            key == wsy::VirtualKey_Right ||
            key == wsy::VirtualKey_Space)
        {
            IFC_RETURN(OpenSubMenu());
            IFC_RETURN(args->put_Handled(TRUE));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnKeyUp(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnKeyUp.", this));
#endif // CMH_DEBUG

    IFC_RETURN(UpdateOwnerVisualState());
    IFC_RETURN(args->put_Handled(TRUE));
    return S_OK;
}

// Creates a DispatcherTimer for delaying showing the sub menu flyout
_Check_return_ HRESULT CascadingMenuHelper::EnsureDelayOpenMenuTimer()
{
    if (!m_delayOpenMenuTimer.Get())
    {
        ctl::ComPtr<DispatcherTimer> dispatcherTimer;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> delayOpenMenuTimerTickHandler;
        EventRegistrationToken delayOpenMenuTimerTickToken;
        wf::TimeSpan delayTimeSpan = {};

        IFC_RETURN(ctl::make<DispatcherTimer>(&dispatcherTimer));

        delayOpenMenuTimerTickHandler.Attach(
            new ClassMemberEventHandler<
            CascadingMenuHelper,
            IInspectable,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &CascadingMenuHelper::DelayOpenMenuTimerTickHandler));
        IFC_RETURN(dispatcherTimer->add_Tick(delayOpenMenuTimerTickHandler.Get(), &delayOpenMenuTimerTickToken));

        delayTimeSpan.Duration = m_subMenuShowDelay * TICKS_PER_MILLISECOND;
        IFC_RETURN(dispatcherTimer->put_Interval(delayTimeSpan));

        SetPtrValue(m_delayOpenMenuTimer, dispatcherTimer.Get());
    }

    return S_OK;
}

// Handler for the Tick event on the delay open menu timer.
_Check_return_ HRESULT CascadingMenuHelper::DelayOpenMenuTimerTickHandler(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: DelayOpenMenuTimerTickHandler.", this));
#endif // CMH_DEBUG

    IFC_RETURN(EnsureCloseExistingSubItems());

    // Open the current sub menu
    IFC_RETURN(OpenSubMenu());

    if (m_delayOpenMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: DelayOpenMenuTimerTickHandler - Stopping m_delayOpenMenuTimer.", this));
#endif // CMH_DEBUG

        IFC_RETURN(m_delayOpenMenuTimer->Stop());
    }

    return S_OK;
}

// Creates a DispatcherTimer for delaying hiding the sub menu flyout
_Check_return_ HRESULT CascadingMenuHelper::EnsureDelayCloseMenuTimer()
{
    if (!m_delayCloseMenuTimer.Get())
    {
        ctl::ComPtr<DispatcherTimer> dispatcherTimer;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> delayCloseMenuTimerTickHandler;
        EventRegistrationToken delayCloseMenuTimerTickToken;
        wf::TimeSpan delayTimeSpan = {};

        IFC_RETURN(ctl::make<DispatcherTimer>(&dispatcherTimer));

        delayCloseMenuTimerTickHandler.Attach(
            new ClassMemberEventHandler<
            CascadingMenuHelper,
            IInspectable,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &CascadingMenuHelper::DelayCloseMenuTimerTickHandler));
        IFC_RETURN(dispatcherTimer->add_Tick(delayCloseMenuTimerTickHandler.Get(), &delayCloseMenuTimerTickToken));

        delayTimeSpan.Duration = m_subMenuShowDelay * TICKS_PER_MILLISECOND;
        IFC_RETURN(dispatcherTimer->put_Interval(delayTimeSpan));

        SetPtrValue(m_delayCloseMenuTimer, dispatcherTimer.Get());
    }

    return S_OK;
}

// Handler for the Tick event on the delay close menu timer.
_Check_return_ HRESULT CascadingMenuHelper::DelayCloseMenuTimerTickHandler(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: DelayCloseMenuTimerTickHandler.", this));
#endif // CMH_DEBUG

    IFC_RETURN(CloseSubMenu());

    if (m_delayCloseMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: DelayCloseMenuTimerTickHandler - Stopping m_delayCloseMenuTimer.", this));
#endif // CMH_DEBUG
        IFC_RETURN(m_delayCloseMenuTimer->Stop());
    }

    return S_OK;
}

// Ensure that any currently open sub menus are closed
_Check_return_ HRESULT CascadingMenuHelper::EnsureCloseExistingSubItems()
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: EnsureCloseExistingSubItems.", this));
#endif // CMH_DEBUG

    ctl::ComPtr<ISubMenuOwner> ownerAsSubMenuOwner;
    IFC_RETURN(m_wpOwner.As(&ownerAsSubMenuOwner));

    if (ownerAsSubMenuOwner)
    {
        IFC_RETURN(ownerAsSubMenuOwner->ClosePeerSubMenus());
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::SetSubMenuPresenter(_In_ xaml::IFrameworkElement* subMenuPresenter)
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: SetSubMenuPresenter.", this));
#endif // CMH_DEBUG

    IFC_RETURN(ctl::AsWeak(subMenuPresenter, &m_wpSubMenuPresenter));

    ctl::ComPtr<ISubMenuOwner> ownerAsSubMenuOwner;
    IFC_RETURN(m_wpOwner.As(&ownerAsSubMenuOwner));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: SetSubMenuPresenter - ownerAsSubMenuOwner=0x%p.", this, ownerAsSubMenuOwner));
#endif // CMH_DEBUG

    if (ownerAsSubMenuOwner)
    {
        ctl::ComPtr<IMenuPresenter> menuPresenter = ctl::query_interface_cast<IMenuPresenter>(subMenuPresenter);

#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: SetSubMenuPresenter - menuPresenter=0x%p.", this, menuPresenter));
#endif // CMH_DEBUG

        if (menuPresenter)
        {
            IFC_RETURN(menuPresenter->put_Owner(ownerAsSubMenuOwner.Get()));
        }
    }

    return S_OK;
}

// Shows the sub menu at the appropriate position.
// The sub menu will be adjusted if the sub presenter size changes.
_Check_return_ HRESULT CascadingMenuHelper::OpenSubMenu()
{
    ctl::ComPtr<ISubMenuOwner> ownerAsSubMenuOwner;
    IFC_RETURN(m_wpOwner.As(&ownerAsSubMenuOwner));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OpenSubMenu - ownerAsSubMenuOwner=0x%p.", this, ownerAsSubMenuOwner));
#endif // CMH_DEBUG

    if (ownerAsSubMenuOwner)
    {
        IFC_RETURN(ownerAsSubMenuOwner->PrepareSubMenu());

        BOOLEAN isSubMenuOpen = FALSE;
        IFC_RETURN(ownerAsSubMenuOwner->get_IsSubMenuOpen(&isSubMenuOpen));

        if (!isSubMenuOpen)
        {
            ctl::ComPtr<Control> ownerAsControl;
            IFC_RETURN(m_wpOwner.As(&ownerAsControl));

            if (ownerAsControl)
            {
                IFC_RETURN(EnsureCloseExistingSubItems());

                double subItemWidth = 0;
                IFC_RETURN(ownerAsControl->get_ActualWidth(&subItemWidth));

                xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
                IFC_RETURN(ownerAsControl->get_FlowDirection(&flowDirection));

                wf::Point subMenuPosition = {};

                BOOLEAN isPositionedAbsolutely = FALSE;
                IFC_RETURN(ownerAsSubMenuOwner->get_IsSubMenuPositionedAbsolutely(&isPositionedAbsolutely));

                if (isPositionedAbsolutely)
                {
                    ctl::ComPtr<xaml_media::IGeneralTransform> transformToRoot;
                    IFC_RETURN(ownerAsControl->TransformToVisual(nullptr, &transformToRoot));
                    IFC_RETURN(transformToRoot->TransformPoint(subMenuPosition, &subMenuPosition));
                }

                ctl::ComPtr<MenuFlyoutSubItem> ownerAsMenuFlyoutSubItem;
                IGNOREHR(ownerAsControl.As(&ownerAsMenuFlyoutSubItem));
                Popup* popup = nullptr;
                Control* menuFlyoutPresenter = nullptr;

                if (ownerAsMenuFlyoutSubItem)
                {
                    popup = ownerAsMenuFlyoutSubItem->GetPopup();
                    menuFlyoutPresenter = ownerAsMenuFlyoutSubItem->GetMenuFlyoutPresenter();

                    VisualTree* visualTree = VisualTree::GetForElementNoRef(ownerAsMenuFlyoutSubItem->GetHandle());
                    if (visualTree)
                    {
                        // Put the popup on the same VisualTree as this flyout sub item to make sure it shows up in the right place
                        static_cast<CPopup*>(popup->GetHandle())->SetAssociatedVisualTree(visualTree);
                    }
                }

                if (flowDirection == xaml::FlowDirection_RightToLeft && isPositionedAbsolutely)
                {
                    subMenuPosition.X += static_cast<float>(m_subMenuOverlapPixels - subItemWidth);
                }
                else
                {
                    subMenuPosition.X += static_cast<float>(subItemWidth - m_subMenuOverlapPixels);
                }

                if (popup && menuFlyoutPresenter)
                {
                    double menuFlyoutPresenterWidth = 0;
                    double menuFlyoutPresenterHeight = 0;
                    IFC_RETURN(menuFlyoutPresenter->get_ActualWidth(&menuFlyoutPresenterWidth));
                    IFC_RETURN(menuFlyoutPresenter->get_ActualHeight(&menuFlyoutPresenterHeight));

                    wf::Point subMenuPosition2 = {};
                    bool isSubMenuDirectionUp = false;
                    bool positionAndDirectionSet = false;

                    // GetPositionAndDirection is called to identify the submenu's direction alone.
                    IFC_RETURN(GetPositionAndDirection(
                        static_cast<float>(menuFlyoutPresenterWidth),
                        static_cast<float>(menuFlyoutPresenterHeight),
                        popup,
                        subMenuPosition2,
                        &isSubMenuDirectionUp,
                        &positionAndDirectionSet));

                    ASSERT(subMenuPosition2.X == -std::numeric_limits<float>::infinity() || subMenuPosition2.X == subMenuPosition.X);
                    ASSERT(subMenuPosition2.Y == -std::numeric_limits<float>::infinity() || subMenuPosition2.Y == subMenuPosition.Y);

                    if (positionAndDirectionSet)
                    {
                        IFC_RETURN(ownerAsSubMenuOwner->SetSubMenuDirection(isSubMenuDirectionUp));
                    }
                }

                IFC_RETURN(ownerAsSubMenuOwner->OpenSubMenu(subMenuPosition));                
                IFC_RETURN(ownerAsSubMenuOwner->RaiseAutomationPeerExpandCollapse(TRUE /* isOpen */));
                IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Invoke, ownerAsControl.Get()));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::CloseSubMenu()
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: CloseSubMenu.", this));
#endif // CMH_DEBUG

    IFC_RETURN(CloseChildSubMenus());

    ctl::ComPtr<ISubMenuOwner> owner;
    IFC_RETURN(m_wpOwner.As(&owner));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: CloseSubMenu - owner=0x%p.", this, owner));
#endif // CMH_DEBUG

    if (owner)
    {
        IFC_RETURN(owner->CloseSubMenu());
        IFC_RETURN(owner->RaiseAutomationPeerExpandCollapse(FALSE /* isOpen */));

        ctl::ComPtr<xaml::IDependencyObject> ownerAsDO = owner.AsOrNull<xaml::IDependencyObject>();

        if (ownerAsDO)
        {
            IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Hide, ownerAsDO.Cast<DependencyObject>()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::CloseChildSubMenus()
{
    // WeakRefPtr fails an assert if we attempt to AsOrNull() to a type
    // that we aren't sure if the contents of the weak reference
    // implement that type.  To avoid that assert, we first As() the
    // weak reference contents to a known type that it's guaranteed
    // to be (if it isn't null), and we then AsOrNull() the ComPtr,
    // once it's safe to ask about a type that we're not sure of.
    ctl::ComPtr<IFrameworkElement> subMenuPresenterAsFE;
    IFC_RETURN(m_wpSubMenuPresenter.As(&subMenuPresenterAsFE));

    ctl::ComPtr<IMenuPresenter> subMenuPresenter;

    if (subMenuPresenterAsFE)
    {
        subMenuPresenter = subMenuPresenterAsFE.AsOrNull<IMenuPresenter>();
    }

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: CloseChildSubMenus - subMenuPresenter=0x%p.", this, subMenuPresenter));
#endif // CMH_DEBUG

    if (subMenuPresenter)
    {
        IFC_RETURN(subMenuPresenter->CloseSubMenu());
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::DelayCloseSubMenu()
{
    IFC_RETURN(EnsureDelayCloseMenuTimer());
    if (m_delayCloseMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: DelayCloseSubMenu - Starting m_delayCloseMenuTimer.", this));
#endif // CMH_DEBUG

        IFC_RETURN(m_delayCloseMenuTimer->Start());
        IFC_RETURN(UpdateOwnerVisualState());
    }
    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::CancelCloseSubMenu()
{
    if (m_delayCloseMenuTimer)
    {
#ifdef CMH_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: CancelCloseSubMenu - Stopping m_delayCloseMenuTimer.", this));
#endif // CMH_DEBUG

        IFC_RETURN(m_delayCloseMenuTimer->Stop());
    }
    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::ClearStateFlags()
{
#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: ClearStateFlags.", this));
#endif // CMH_DEBUG

    m_isPressed = false;
    m_isPointerOver = false;
    IFC_RETURN(UpdateOwnerVisualState());
    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* args)
{
    ctl::ComPtr<Control> ownerAsControl;
    IFC_RETURN(m_wpOwner.As(&ownerAsControl));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnIsEnabledChanged - ownerAsControl=0x%p.", this, ownerAsControl));
#endif // CMH_DEBUG

    if (ownerAsControl)
    {
        BOOLEAN bIsEnabled = FALSE;
        IFC_RETURN(ownerAsControl->get_IsEnabled(&bIsEnabled));

        if (!bIsEnabled)
        {
            IFC_RETURN(ClearStateFlags());
        }
        else
        {
            IFC_RETURN(Control::UpdateVisualState(ownerAsControl.Get(), true /* useTransitions */));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnVisibilityChanged()
{
    ctl::ComPtr<IUIElement> ownerAsUIE;
    IFC_RETURN(m_wpOwner.As(&ownerAsUIE));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnVisibilityChanged - ownerAsUIE=0x%p.", this, ownerAsUIE));
#endif // CMH_DEBUG

    if (ownerAsUIE)
    {
        xaml::Visibility visibility = xaml::Visibility_Collapsed;
        IFC_RETURN(ownerAsUIE->get_Visibility(&visibility));

        if (xaml::Visibility_Visible != visibility)
        {
            IFC_RETURN(ClearStateFlags());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::OnPresenterSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* args,
    _In_ Popup* popup)
{
    ctl::ComPtr<ISubMenuOwner> ownerAsSubMenuOwner;
    IFC_RETURN(m_wpOwner.As(&ownerAsSubMenuOwner));

#ifdef CMH_DEBUG
    IGNOREHR(DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: OnPresenterSizeChanged - ownerAsSubMenuOwner=0x%p.", this, ownerAsSubMenuOwner));
#endif // CMH_DEBUG

    if (ownerAsSubMenuOwner)
    {
        wf::Point subMenuPosition = {};
        wf::Size newPresenterSize = {};
        bool isSubMenuDirectionUp = false;
        bool positionAndDirectionSet = false;

        IFC_RETURN(args->get_NewSize(&newPresenterSize));
        IFC_RETURN(GetPositionAndDirection(
            newPresenterSize.Width,
            newPresenterSize.Height,
            popup,
            subMenuPosition,
            &isSubMenuDirectionUp,
            &positionAndDirectionSet));

        if (positionAndDirectionSet)
        {
            IFC_RETURN(ownerAsSubMenuOwner->PositionSubMenu(subMenuPosition));
            IFC_RETURN(ownerAsSubMenuOwner->SetSubMenuDirection(isSubMenuDirectionUp));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::GetPositionAndDirection(
    float presenterWidth,
    float presenterHeight,
    _In_ Popup* popup,
    _Out_ wf::Point& subMenuPosition,
    _Out_ bool* isSubMenuDirectionUp,
    _Out_ bool* positionAndDirectionSet)
{
    ASSERT(popup);
    ASSERT(isSubMenuDirectionUp);
    ASSERT(positionAndDirectionSet);

    // We sometimes will only want to change one of the two XY-positions of the menu,
    // but some menus (e.g. AppBarButton.Flyout) don't allow you to individually change
    // one axis of the position - you need to close and reopen the menu in a different location.
    // This necessitates a single function call that takes a Point parameter rather than
    // two function calls that individually change the X and then the Y position of the menu,
    // since otherwise we'd be closing and reopening the menu twice if we needed to change
    // both positions, which would be visually disruptive.
    // As such, we need a way to tell PositionSubMenu to leave one of the positions as it was.
    // We'll use negative infinity as a sentinel value that means "don't change this coordinate value".
    subMenuPosition = { -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };

    *isSubMenuDirectionUp = false;
    *positionAndDirectionSet = false;

    ctl::ComPtr<Control> ownerAsControl;
    IFC_RETURN(m_wpOwner.As(&ownerAsControl));

    ctl::ComPtr<ISubMenuOwner> ownerAsSubMenuOwner;
    IFC_RETURN(m_wpOwner.As(&ownerAsSubMenuOwner));

    if (!ownerAsControl || !ownerAsSubMenuOwner)
    {
        return S_OK;
    }

    BOOLEAN isPositionedAbsolutely = FALSE;
    IFC_RETURN(ownerAsSubMenuOwner->get_IsSubMenuPositionedAbsolutely(&isPositionedAbsolutely));

    wf::Point ownerPosition = {};

    if (isPositionedAbsolutely)
    {
        // Get the current sub menu owner position as the client point
        ctl::ComPtr<xaml_media::IGeneralTransform> transformToRoot;
        IFC_RETURN(ownerAsControl->TransformToVisual(nullptr, &transformToRoot));
        IFC_RETURN(transformToRoot->TransformPoint(ownerPosition, &ownerPosition));
    }

    wf::Rect availableBounds = {};

    // If the current menu is a windowed popup, the position setting will be within the current nearest
    // monitor boundary. Otherwise, the position will be within the Xaml window boundary.
    if (static_cast<CPopup*>(popup->GetHandle())->IsWindowed())
    {
        // Get the available monitor bounds from the current nearest monitor.
        // IHM bounds will be excluded from the monitor bounds.
        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(popup, ownerPosition, &availableBounds));
    }
    else
    {
        // Get the available window rect
        IFC_RETURN(FlyoutBase::CalculateAvailableWindowRect(
            popup,
            nullptr /* placementTarget */,
            true    /* hasTargetPosition */,
            ownerPosition,
            false   /* isFull */,
            &availableBounds));
    }

    IFC_RETURN(GetPositionAndDirection(
        presenterWidth,
        presenterHeight,
        availableBounds,
        ownerPosition,
        subMenuPosition,
        isSubMenuDirectionUp,
        positionAndDirectionSet));

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::GetPositionAndDirection(
    float presenterWidth,
    float presenterHeight,
    const wf::Rect& availableBounds,
    const wf::Point& ownerPosition,
    _Out_ wf::Point& subMenuPosition,
    _Out_ bool* isSubMenuDirectionUp,
    _Out_ bool* positionAndDirectionSet)
{
#ifdef CMH_DEBUG
    IGNOREHR(DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: GetPositionAndDirection - entry. presenterWidth=%f, presenterHeight=%f, ownerPosition=(%f, %f), availableBounds=(%f, %f, %f, %f).",
        this, presenterWidth, presenterHeight, ownerPosition.X, ownerPosition.Y, availableBounds.X, availableBounds.Y, availableBounds.Width, availableBounds.Height));
#endif // CMH_DEBUG

    ASSERT(isSubMenuDirectionUp);
    ASSERT(positionAndDirectionSet);

    subMenuPosition = { -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };

    *isSubMenuDirectionUp = false;
    *positionAndDirectionSet = false;

    ctl::ComPtr<Control> ownerAsControl;
    IFC_RETURN(m_wpOwner.As(&ownerAsControl));

    ctl::ComPtr<Control> presenterAsControl;
    IFC_RETURN(m_wpSubMenuPresenter.As(&presenterAsControl));

    if (!ownerAsControl || !presenterAsControl)
    {
        return S_OK;
    }

    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(ownerAsControl->get_FlowDirection(&flowDirection));

    // Get the current sub menu item width and height
    double ownerWidth = 0;
    double ownerHeight = 0;
    IFC_RETURN(ownerAsControl->get_ActualWidth(&ownerWidth));
    IFC_RETURN(ownerAsControl->get_ActualHeight(&ownerHeight));

    // Get the current presenter max width/height
    double maxWidth = DoubleUtil::NaN;
    double maxHeight = DoubleUtil::NaN;
    IFC_RETURN(presenterAsControl->get_MaxWidth(&maxWidth));
    IFC_RETURN(presenterAsControl->get_MaxHeight(&maxHeight));

    // Set the max width and height with the available bounds
    IFC_RETURN(presenterAsControl->put_MaxWidth(
        DoubleUtil::IsNaN(maxWidth) ? availableBounds.Width : DoubleUtil::Min(maxWidth, availableBounds.Width)));
    IFC_RETURN(presenterAsControl->put_MaxHeight(
        DoubleUtil::IsNaN(maxHeight) ? availableBounds.Height : DoubleUtil::Min(maxHeight, availableBounds.Height)));

    // Get the available bottom space from the current available bounds
    const double bottomSpace = availableBounds.Y + availableBounds.Height - ownerPosition.Y;

    if (flowDirection == xaml::FlowDirection_LeftToRight)
    {
        // Get the available right space from the current available bounds
        const double rightSpace = availableBounds.X + availableBounds.Width - ownerPosition.X - ownerWidth;
        // If the current sub presenter width isn't enough in the default right space,
        // the MenuFlyoutSubItem will be positioned on the left side if the current presenter
        // width is less than the sub item left(X) position. Otherwise, it will be aligned to
        // the right side of the available bounds.
        if (presenterWidth > rightSpace)
        {
            if (presenterWidth < availableBounds.Width - rightSpace - ownerWidth)
            {
                subMenuPosition.X = static_cast<float>(ownerPosition.X - presenterWidth + m_subMenuOverlapPixels);
            }
            else
            {
                subMenuPosition.X = static_cast<float>(ownerPosition.X + ownerWidth + rightSpace - presenterWidth);
            }
        }
    }
    else
    {
        // Get the available left space from the current available bounds
        const double leftSpace = ownerPosition.X - availableBounds.X - ownerWidth;
        // If the current sub presenter width isn't enough in the default left space,
        // the MenuFlyoutSubItem will be positioned on the right side if the current presenter
        // width is less than the sub item right(X) position. Otherwise, it will be aligned to
        // the left side of the available monitor rect.
        // the left side of the available bounds.
        if (presenterWidth > leftSpace)
        {
            if (presenterWidth < (availableBounds.Width - leftSpace - ownerWidth))
            {
                subMenuPosition.X = static_cast<float>(ownerPosition.X + presenterWidth - m_subMenuOverlapPixels);
            }
            else
            {
                subMenuPosition.X = static_cast<float>(ownerPosition.X - ownerWidth - leftSpace + presenterWidth);
            }
        }
        else
        {
            subMenuPosition.X = static_cast<float>(ownerPosition.X - ownerWidth + m_subMenuOverlapPixels);
        }
    }

    // If the current sub presenter doesn't have space to fit in the default bottom position,
    // then the MenuFlyoutSubItem will be aligned with the bottom of the available bounds.
    // If the MenuFlyoutSubItem is too tall to fit when bottom aligned with the available bounds
    // then it will be top or bottom aligned with the edge of the available bounds.
    if (presenterHeight > bottomSpace)
    {
        // There is not enough bottom space to align top of owner with top of presenter.
        const double topSpace = availableBounds.Height + ownerHeight - bottomSpace;

        if (topSpace >= presenterHeight)
        {
            // There is enough top space to align bottom of owner with bottom of presenter.
            subMenuPosition.Y = static_cast<float>(ownerPosition.Y + ownerHeight - presenterHeight);
            *isSubMenuDirectionUp = true;
        }
        else
        {
            // presenterHeight > bottomSpace and presenterHeight > topSpace
            if (bottomSpace < topSpace)
            {
                // Aligning top of presenter with top of available bounds.
                subMenuPosition.Y = availableBounds.Y;
                *isSubMenuDirectionUp = true;
            }
            else
            {
                // Aligning top of presenter with top of available bounds.
                subMenuPosition.Y = availableBounds.Y;
                *isSubMenuDirectionUp = true;            
            }
        }
    }
    else
    {
        // There is enough bottom space to align top of owner with top of presenter.
        subMenuPosition.Y = ownerPosition.Y;
    }

    *positionAndDirectionSet = true;

#ifdef CMH_DEBUG
    IGNOREHR(DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: GetPositionAndDirection - exit. subMenuPosition=(%f, %f), isSubMenuDirectionUp=%d.", this, subMenuPosition.X, subMenuPosition.Y, *isSubMenuDirectionUp));
#endif // CMH_DEBUG
        
    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::UpdateOwnerVisualState()
{
    ctl::ComPtr<Control> ownerAsControl;
    IFC_RETURN(m_wpOwner.As(&ownerAsControl));

#ifdef CMH_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"CMH[0x%p]: UpdateOwnerVisualState - ownerAsControl=0x%p.", this, ownerAsControl));
#endif // CMH_DEBUG

    if (ownerAsControl)
    {
        IFC_RETURN(Control::UpdateVisualState(ownerAsControl.Get(), true /* useTransitions */));
    }

    return S_OK;
}

_Check_return_ HRESULT CascadingMenuHelper::IsDelayCloseTimerRunning(
            _Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    if(m_delayCloseMenuTimer)
    {
        IFC_RETURN(m_delayCloseMenuTimer->get_IsEnabled(pValue));
    }
    return S_OK;
}
