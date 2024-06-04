// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "AutomationPeer.g.h"
#include "FrameworkApplication.g.h"
#include "Rectangle.g.h"
#include "Popup.g.h"
#include "SolidColorBrush.g.h"
#include "Window.g.h"
#include "Storyboard.g.h"
#include "Canvas.g.h"
#include "ScaleTransform.g.h"
#include "Image.g.h"
#include "RenderTargetBitmap.g.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "InputServices.h"
#include "namespacealiases.h"
#include "VisualTreeHelper.h"
#include "MenuFlyoutItemBase.g.h"
#include "FlyoutShowOptions.g.h"
#include "ElevationHelper.h"
#include "WrlHelper.h"
#include "XamlTelemetry.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

MenuFlyout::MenuFlyout()
{
    m_isPositionedAtPoint = true;
    m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::None;
}

// Prepares object's state
_Check_return_ HRESULT MenuFlyout::PrepareState()
{
    ctl::ComPtr<MenuFlyoutItemBaseCollection> spItems;

    IFC_RETURN(MenuFlyoutGenerated::PrepareState());

    IFC_RETURN(ctl::make(&spItems));
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spItems->GetHandle()), GetHandle()));
    SetPtrValue(m_tpItems, spItems);

    IFC_RETURN(put_Items(spItems.Get()));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::DisconnectFrameworkPeerCore()
{
    HRESULT hr = S_OK;

    //
    // DXAML Structure
    // ---------------
    //   DXAML::MenuFlyout (this)   -------------->   DXAML::MenuFlyoutItemBase
    //      |                           |
    //      |                           +--------->   DXAML::MenuFlyoutItemBase
    //      V
    //   DXAML::MenuFlyoutItemBaseCollection (m_tpItems)
    //
    // CORE Structure
    // --------------
    //   Core::CControl (this)              < - - +         < - - +
    //            |                               :               :
    //            V                               :               :
    //   Core::CMenuFlyoutItemBaseCollection  - - + (m_pOwner)    :
    //      |                  |                                  :
    //      V                  V                                  :
    //   Core::CControl   Core::CControl      - - - - - - - - - - + (m_pParent)
    //
    // To clear the m_pParent association of the MenuFlyoutItemBase, we have to clear the
    // Core::CMenuFlyoutItemBaseCollection's children, which calls SetParent(NULL) on each of its
    // children. Once this association to MenuFlyout is broken, we can safely destroy MenuFlyout.
    //

    // clear the children in the MenuFlyoutItemBaseCollection
    if (m_tpItems.GetAsCoreDO() != nullptr)
    {
       IFC(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpItems.GetAsCoreDO())));
       IFC(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpItems.GetAsCoreDO()), nullptr));
    }

    IFC(MenuFlyoutGenerated::DisconnectFrameworkPeerCore());

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods.
_Check_return_ HRESULT MenuFlyout::OnPropertyChanged2(const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutGenerated::OnPropertyChanged2(args));

    if (KnownPropertyIndex::MenuFlyout_MenuFlyoutPresenterStyle == args.m_pDP->GetIndex() &&
        GetPresenter() != NULL)
    {
        ctl::ComPtr<IInspectable> spInspectable;
        ctl::ComPtr<IStyle> spStyle;
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spInspectable));
        IFC(spInspectable.As(&spStyle));
        IFC(SetPresenterStyle(GetPresenter(), spStyle.Get()));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyout::CreatePresenter(
    _Outptr_ IControl** ppReturnValue)
{
    ctl::ComPtr<MenuFlyoutPresenter> presenter;
    ctl::ComPtr<IStyle> style;

    *ppReturnValue = NULL;

    IFC_RETURN(ctl::make(&presenter));
    IFC_RETURN(presenter->SetParentMenuFlyout(this));

    IFC_RETURN(get_MenuFlyoutPresenterStyle(&style));
    IFC_RETURN(SetPresenterStyle(presenter.Get(), style.Get()));

    IFC_RETURN(presenter.MoveTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::ShowAtCore(
    _In_ xaml::IFrameworkElement* pPlacementTarget,
    _Out_ bool& openDelayed)
{
    openDelayed = false;

    if (m_openWindowed)
    {
        IFC_RETURN(SetIsWindowedPopup());
    }
    else
    {
        m_openWindowed = true;
    }

    ctl::ComPtr<IFrameworkElement> placementTarget(pPlacementTarget);
    IFC_RETURN(CacheInputDeviceTypeUsedToOpen(static_cast<CUIElement*>(placementTarget.AsOrNull<DependencyObject>()->GetHandle())));

    IFC_RETURN(MenuFlyoutGenerated::ShowAtCore(pPlacementTarget, openDelayed));

    return S_OK;
}

// Raise Opening event.
_Check_return_ HRESULT MenuFlyout::OnOpening()
{
    // Update the TemplateSettings as it is about to open.
    ctl::ComPtr<IControl> presenter = GetPresenter();
    ctl::ComPtr<MenuFlyoutPresenter> menuFlyoutPresenter = presenter.AsOrNull<MenuFlyoutPresenter>();

    if (menuFlyoutPresenter)
    {
        ctl::ComPtr<IMenu> parentMenu;
        IFC_RETURN(get_ParentMenu(&parentMenu));

        IFC_RETURN(menuFlyoutPresenter->put_OwningMenu(parentMenu ? parentMenu.Get() : this));
        IFC_RETURN(menuFlyoutPresenter->UpdateTemplateSettings());
    }

    IFC_RETURN(MenuFlyoutGenerated::OnOpening());

    // Reset the presenter's ItemsSource.  Since MenuFlyout.Items is not an IObservableVector, we don't
    // automatically respond to changes within the vector.  Clearing the property when the presenter
    // unloads and resetting it before we reopen ensures any changes to MenuFlyout.Items are reflected
    // when the MenuFlyoutPresenter shows.  It also allows sharing of MenuFlyouts; since MenuFlyoutItemBases
    // are UIElements they must be unparented when leaving the tree before they can be inserted elsewhere.
    IFC_RETURN(menuFlyoutPresenter->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

    IFC_RETURN(AutomationPeer::RaiseEventIfListener(menuFlyoutPresenter.Get(), xaml_automation_peers::AutomationEvents_MenuOpened));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::OnClosing(bool* cancel)
{
    IFC_RETURN(__super::OnClosing(cancel));

    if (!(*cancel))
    {
        IFC_RETURN(CloseSubMenu());
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::OnClosed()
{
    IFC_RETURN(CloseSubMenu());

    IFC_RETURN(AutomationPeer::RaiseEventIfListener(static_cast<MenuFlyoutPresenter*>(GetPresenter()), xaml_automation_peers::AutomationEvents_MenuClosed));

    IFC_RETURN(MenuFlyoutGenerated::OnClosed());

    static_cast<MenuFlyoutPresenter*>(GetPresenter())->m_iFocusedIndex = -1;
    IFC_RETURN(static_cast<ItemsControl*>(GetPresenter())->put_ItemsSource(nullptr));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::CloseSubMenu()
{
    MenuFlyoutPresenter* presenter = static_cast<MenuFlyoutPresenter*>(GetPresenter());

    if (presenter)
    {
        ctl::ComPtr<IMenuPresenter> subPresenter;
        IFC_RETURN(presenter->get_SubPresenter(&subPresenter));

        if (subPresenter)
        {
            IFC_RETURN(subPresenter->CloseSubMenu());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::PreparePopupTheme(
    _In_ Popup* pPopup,
    MajorPlacementMode placementMode,
    _In_ xaml::IFrameworkElement* pPlacementTarget)
{
    BOOLEAN areOpenCloseAnimationsEnabled = FALSE;
    IFC_RETURN(get_AreOpenCloseAnimationsEnabled(&areOpenCloseAnimationsEnabled));

    if (!areOpenCloseAnimationsEnabled)
    {
        return S_OK;
    }

    double openedLength = 0;
    xaml_primitives::AnimationDirection direction = xaml_primitives::AnimationDirection_Bottom;

    if (!m_tpMenuPopupThemeTransition)
    {
        ctl::ComPtr<xaml_animation::ITransition> spMenuPopupChildTransition;
        IFC_RETURN(MenuFlyout::PreparePopupThemeTransitionsAndShadows(pPopup, 0.5 /* closedRatioConstant */, 0 /* depth */, &spMenuPopupChildTransition));
        SetPtrValue(m_tpMenuPopupThemeTransition, spMenuPopupChildTransition.Get());
    }

    IFC_RETURN(static_cast<Control*>(GetPresenter())->get_ActualHeight(&openedLength));
    IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_OpenedLength(openedLength));
    direction = (placementMode == MajorPlacementMode::Top) ? xaml_primitives::AnimationDirection_Bottom : xaml_primitives::AnimationDirection_Top;
    IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_Direction(direction));

    return S_OK;
}


/*static*/
_Check_return_ HRESULT MenuFlyout::PreparePopupThemeTransitionsAndShadows(
    _In_ Popup* popup,
    double closedRatioConstant,
    UINT depth,
    _Outptr_ xaml_animation::ITransition** transition)
{
    ctl::ComPtr<ITransition> spTransition;
    ctl::ComPtr<TransitionCollection> spTransitionCollection;
    ctl::ComPtr<MenuPopupThemeTransition> spMenuPopupChildTransition;

    *transition = nullptr;

    IFC_RETURN(ctl::make(&spMenuPopupChildTransition));
    IFC_RETURN(ctl::make(&spTransitionCollection));

    IFC_RETURN(spMenuPopupChildTransition->put_ClosedRatio(closedRatioConstant));

    ctl::ComPtr<xaml::IFrameworkElement> overlayElement;
    IFC_RETURN(popup->get_OverlayElement(&overlayElement));
    spMenuPopupChildTransition->SetOverlayElement(overlayElement.Get());

    IFC_RETURN(spMenuPopupChildTransition.As(&spTransition));
    IFC_RETURN(spTransitionCollection->Append(spTransition.Get()));

    // For windowed popups, the transition needs to target the grandchild of the popup.
    // Otherwise, the transition LTE is going to live in the main window and get clipped.
    if (static_cast<CPopup*>(popup->GetHandle())->IsWindowed())
    {
        ctl::ComPtr<IUIElement> popupChild;
        IFC_RETURN(popup->get_Child(&popupChild));

        if (popupChild)
        {
            int childrenCount;
            IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(static_cast<UIElement*>(popupChild.Get()), &childrenCount));

            if (childrenCount == 1)
            {
                ctl::ComPtr<xaml::IDependencyObject> popupGrandChildAsDO;
                IFC_RETURN(VisualTreeHelper::GetChildStatic(static_cast<UIElement*>(popupChild.Get()), 0, &popupGrandChildAsDO));

                if (popupGrandChildAsDO)
                {
                    ctl::ComPtr<IUIElement> popupGrandChildAsUE;
                    IFC_RETURN(popupGrandChildAsDO.As(&popupGrandChildAsUE));
                    IFC_RETURN(popupGrandChildAsUE->put_Transitions(spTransitionCollection.Get()));
                    IFC_RETURN(popupGrandChildAsUE->InvalidateMeasure());
                }
            }
        }
    }
    else
    {
        IFC_RETURN(popup->put_ChildTransitions(spTransitionCollection.Get()));
    }

    *transition = spMenuPopupChildTransition.Detach();
    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::UpdatePresenterVisualState(
    FlyoutBase::MajorPlacementMode placement,
    BOOLEAN doForceTransitions)
{
    IFC_RETURN(MenuFlyoutGenerated::UpdatePresenterVisualState(placement));

    // DEAD_CODE_REMOVAL: Execution should never get here, this was just for phone.
    XAML_FAIL_FAST();

    // MenuFlyoutPresenter has different visual states depending on the flyout's placement.
    if (doForceTransitions)
    {
        // In order to play the storyboards of the visual states that belong to the
        // MenuFlyoutPresenter, we need to force a visual state transition.
        IFC_RETURN(static_cast<MenuFlyoutPresenter*>(GetPresenter())->ResetVisualState());
    }

    IFC_RETURN(static_cast<MenuFlyoutPresenter*>(GetPresenter())->UpdateVisualStateForPlacement(placement));
    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::AutoAdjustPlacement(
    _Inout_ FlyoutBase::MajorPlacementMode* pPlacement)
{
    HRESULT hr = S_OK;
    wf::Rect windowRect = {};

    IFC(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowRect));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyout::UpdatePresenterVisualState(
    FlyoutBase::MajorPlacementMode placement)
{
    RRETURN(UpdatePresenterVisualState(placement, TRUE /*doForceTransitions*/));
}

_Check_return_ HRESULT
MenuFlyout::ShowAt(
    _In_ xaml::IFrameworkElement* pPlacementTarget)
{
    bool ignoredOpenDelayed = false;

    m_openWindowed = false;
    IFC_RETURN(ShowAtCore(pPlacementTarget, ignoredOpenDelayed));
    return S_OK;
}

_Check_return_ HRESULT
MenuFlyout::ShowAtImpl(
    _In_opt_ xaml::IUIElement* pTargetElement,
    wf::Point targetPoint)
{
    PerfXamlEvent_RAII perfXamlEvent(reinterpret_cast<uint64_t>(this), "MenuFlyout::ShowAt", true);

    ctl::ComPtr<IUIElement> targetElement = pTargetElement;

    ctl::ComPtr<xaml::IDependencyObject> targetAsDO;
    IFC_RETURN(targetElement.As(&targetAsDO));

    ctl::ComPtr<FlyoutShowOptions> showOptions;
    IFC_RETURN(ctl::make(&showOptions));

    ctl::ComPtr<wf::IReference<wf::Point>> targetPointReference;
    IFC_RETURN(PropertyValue::CreateTypedReference<wf::Point>(targetPoint, &targetPointReference));
    IFC_RETURN(showOptions->put_Position(targetPointReference.Get()));

    ctl::ComPtr<IFlyoutShowOptions> showOptionsAsI;
    IFC_RETURN(showOptions.As(&showOptionsAsI));

    ctl::ComPtr<IFlyoutBase> thisAsFlyoutBase;
    IFC_RETURN(ctl::do_query_interface(thisAsFlyoutBase, this));

    IFC_RETURN(thisAsFlyoutBase->ShowAtWithOptions(targetAsDO.Get(), showOptionsAsI.Get()));

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyout::CacheInputDeviceTypeUsedToOpen(
    _In_ CUIElement *pTargetElement)
{
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pTargetElement);
    m_inputDeviceTypeUsedToOpen = contentRoot->GetInputManager().GetLastInputDeviceType();

    return S_OK;
}

// Callback for ShowAt() from core layer
_Check_return_ HRESULT MenuFlyout::ShowAtStatic(
    _In_ CMenuFlyout* pCoreMenuFlyout,
    _In_ CUIElement* pCoreTarget,
    wf::Point point)
{
    ASSERT(pCoreMenuFlyout);
    ASSERT(pCoreTarget);

    ctl::ComPtr<DependencyObject> menuFlyout;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(reinterpret_cast<CDependencyObject*>(pCoreMenuFlyout), &menuFlyout));
    ASSERT(ctl::is<IMenuFlyout>(menuFlyout));

    ctl::ComPtr<DependencyObject> target;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pCoreTarget, &target));
    ASSERT(ctl::is<IFrameworkElement>(target));

    IFC_RETURN(menuFlyout.Cast<MenuFlyout>()->ShowAtImpl(target.Cast<FrameworkElement>(), point));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    unsigned int itemCount = 0;

    if (m_tpItems != nullptr)
    {
        IFC_RETURN(m_tpItems->get_Size(&itemCount));

        for (unsigned int i = 0; i < itemCount; i++)
        {
            ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase> spItem;
            IFC_RETURN(m_tpItems->GetAt(i, spItem.ReleaseAndGetAddressOf()));
            IFC_RETURN(spItem.Cast<MenuFlyoutItemBase>()->TryInvokeKeyboardAccelerator(pArgs));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::get_ParentMenuImpl(_Outptr_result_maybenull_ IMenu** ppValue)
{
    ctl::ComPtr<IMenu> parentMenu;
    parentMenu = m_wrParentMenu.AsOrNull<IMenu>();
    *ppValue = parentMenu.Detach();

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::put_ParentMenuImpl(_In_opt_ IMenu* pValue)
{
    IFC_RETURN(ctl::AsWeak(pValue, &m_wrParentMenu));

    // If we have a parent menu, then we want to disable the light-dismiss overlay -
    // in that circumstance, the parent menu will have a light-dismiss overlay that we'll use instead.
    IFC_RETURN(put_IsLightDismissOverlayEnabled(!pValue));

    IFC_RETURN(EnsurePopupAndPresenter());

    ctl::ComPtr<IControl> presenter = GetPresenter();
    ctl::ComPtr<MenuFlyoutPresenter> menuFlyoutPresenter;
    IFC_RETURN(presenter.As(&menuFlyoutPresenter));

    const bool isSubMenu = pValue != nullptr;

    IFC_RETURN(m_tpPopup.Cast<Popup>()->put_IsSubMenu(!!isSubMenu));
    menuFlyoutPresenter->m_isSubPresenter = !!isSubMenu;
    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::CloseImpl()
{
    IFC_RETURN(Hide());
    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::QueueRefreshItemsSource()
{
    // The items source might change multiple times in a single tick, so we'll coalesce the refresh
    // into a single event once all of the changes have completed.
    if (GetPresenter() && !m_itemsSourceRefreshPending)
    {
        ctl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
        ctl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            dispatcherQueueStatics.ReleaseAndGetAddressOf()));

        IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(&dispatcherQueue));

        ctl::WeakRefPtr wrThis;
        boolean enqueued = false;

        IFC_RETURN(ctl::ComPtr<IMenuFlyout>(this).AsWeak(&wrThis));

        IFC_RETURN(dispatcherQueue->TryEnqueue(
            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wrThis]() mutable {
                ctl::ComPtr<IMenuFlyout> thisMenuFlyout;
                IFC_RETURN(wrThis.As(&thisMenuFlyout));

                if (thisMenuFlyout)
                {
                    thisMenuFlyout.Cast<MenuFlyout>()->RefreshItemsSource();
                }

                return S_OK;
            }).Get(),
            &enqueued));

        IFCEXPECT_RETURN(enqueued);
        m_itemsSourceRefreshPending = true;
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyout::RefreshItemsSource()
{
    m_itemsSourceRefreshPending = false;

    ctl::ComPtr<IControl> presenter = GetPresenter();

    ASSERT(presenter);

    ctl::ComPtr<MenuFlyoutPresenter> menuFlyoutPresenter;
    IFC_RETURN(presenter.As(&menuFlyoutPresenter));

    // Setting the items source to null and then back to Items causes the presenter to pick up any changes.
    IFC_RETURN(menuFlyoutPresenter->put_ItemsSource(nullptr));
    IFC_RETURN(menuFlyoutPresenter->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

    return S_OK;
}