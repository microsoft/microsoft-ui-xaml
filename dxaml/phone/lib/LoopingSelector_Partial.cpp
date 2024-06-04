// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <WRLHelper.h>

#include <wil\resource.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

const WCHAR LoopingSelector::c_scrollViewerTemplatePart[] = L"ScrollViewer";
const WCHAR LoopingSelector::c_upButtonTemplatePart[] = L"UpButton";
const WCHAR LoopingSelector::c_downButtonTemplatePart[] = L"DownButton";
const DOUBLE LoopingSelector::c_targetScreenWidth = 400.0;

//#define LSTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define LSTRACE(...)

LoopingSelector::LoopingSelector()
    : _hasFocus(FALSE)
    , _isSized(FALSE)
    , _isSetupPending(TRUE)
    , _isScrollViewerInitialized(FALSE)
    , _skipNextBalance(FALSE)
    , _skipSelectionChangeUntilFinalViewChanged(false)
    , _skipNextArrange(FALSE)
    , _itemState(ItemState::Expanded)
    , _unpaddedExtentTop(0.0)
    , _unpaddedExtentBottom(0.0)
    , _realizedTop(0.0)
    , _realizedBottom(0.0)
    , _realizedTopIdx(-1)
    , _realizedBottomIdx(-1)
    , _realizedMidpointIdx(-1)
    , _itemCount(0)
    , _scaledItemHeight(0.0)
    , _itemHeight(0.0)
    , _itemWidth(0.0)
    , _panelSize(0.0)
    , _panelMidpointScrollPosition(0.0)
    , _isWithinScrollChange(FALSE)
    , _isWithinArrangeOverride(FALSE)
    , _disablePropertyChange(FALSE)
    , _lastArrangeSizeHeight(0.0f)
    , _delayScrollPositionY(-1.0)
    , _itemWidthFallback(0.0)
{
    _focusGot.value = 0;
    _focusLost.value = 0;
    _pressedToken.value = 0;
    _viewChangingToken.value = 0;
    _viewChangedToken.value = 0;
    _upButtonClickedToken.value = 0;
    _downButtonClickedToken.value = 0;
    _pointerEnteredToken.value = 0;
    _pointerExitedToken.value = 0;
}

_Check_return_
HRESULT
LoopingSelector::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IControl> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;
    wrl::ComPtr<xaml::IUIElement> spUIElement;

    IFC(LoopingSelectorGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<ILoopingSelector*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.Primitives.LoopingSelector"));

    IFC(QueryInterface(__uuidof(xaml::IUIElement), &spUIElement));

    // These events are automatically removed when the
    // UIElement's destructor is called.
    IFC(spUIElement->add_PointerPressed(
        wrl::Callback<xaml::Input::IPointerEventHandler>
            (this, &LoopingSelector::OnPressed).Get(),
            &_pressedToken));
    IFC(spUIElement->add_LostFocus(
        wrl::Callback<xaml::IRoutedEventHandler>
        (this, &LoopingSelector::OnLostFocus).Get(),
        &_focusLost));
    IFC(spUIElement->add_GotFocus(
        wrl::Callback<xaml::IRoutedEventHandler>
        (this, &LoopingSelector::OnGotFocus).Get(),
        &_focusGot));
    IFC(spUIElement->add_PointerEntered(
        wrl::Callback<xaml::Input::IPointerEventHandler>
        (this, &LoopingSelector::OnPointerEntered).Get(),
        &_pointerEnteredToken));
    IFC(spUIElement->add_PointerExited(
        wrl::Callback<xaml::Input::IPointerEventHandler>
        (this, &LoopingSelector::OnPointerExited).Get(),
        &_pointerExitedToken));

    {
        wrl::ComPtr<IInspectable> spPreviousScrollPosition;
        IFC(Private::ValueBoxer::CreateDouble(0.0, &spPreviousScrollPosition));
        IFC(SetPtrValue(_tpPreviousScrollPosition, spPreviousScrollPosition.Get()));
    }

    // Cache the CanvasStatics because of frequency of use.
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Canvas).Get(),
        &_spCanvasStatics));

Cleanup:
    if (FAILED(hr) && spUIElement)
    {
        if (_pressedToken.value != 0)
        {
            IGNOREHR(spUIElement->remove_PointerPressed(_pressedToken));
        }

        if (_focusLost.value != 0)
        {
            IGNOREHR(spUIElement->remove_LostFocus(_focusLost));
        }

        if (_focusGot.value != 0)
        {
            IGNOREHR(spUIElement->remove_GotFocus(_focusGot));
        }

        if (_pointerEnteredToken.value != 0)
        {
            IGNOREHR(spUIElement->remove_PointerEntered(_pointerEnteredToken));
        }

        if (_pointerExitedToken.value != 0)
        {
            IGNOREHR(spUIElement->remove_PointerExited(_pointerExitedToken));
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::RaiseOnSelectionChanged(
    _In_ IInspectable* pOldItem,
    _In_ IInspectable* pNewItem)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::ISelectionChangedEventArgsFactory> spSelectionChangedEventArgsFactory;
    wrl::ComPtr<xaml_controls::ISelectionChangedEventArgs> spSelectionChangedEventArgs;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spRemovedItems;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spAddedItems;
    wrl::ComPtr<IInspectable> spInner;
    wrl::ComPtr<IInspectable> spThisAsInspectable;

    IFC(wfci_::Vector<IInspectable*>::Make(&spRemovedItems));
    IFC(wfci_::Vector<IInspectable*>::Make(&spAddedItems));

    IFC(spAddedItems->Append(pNewItem));
    IFC(spRemovedItems->Append(pOldItem));

    // Raise the SelectionChanged event
    IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_SelectionChangedEventArgs).Get(),
            &spSelectionChangedEventArgsFactory));

    IFC(spSelectionChangedEventArgsFactory->CreateInstanceWithRemovedItemsAndAddedItems(
           spRemovedItems.Get(),
           spAddedItems.Get(),
           NULL,
           &spInner,
           &spSelectionChangedEventArgs));

    IFC(QueryInterface(__uuidof(IInspectable), &spThisAsInspectable));
    IFC(m_SelectionChangedEventSource.InvokeAll(
             spThisAsInspectable.Get(),
             spSelectionChangedEventArgs.Get()));
Cleanup:
    RRETURN(hr);
}

#pragma region IFrameworkElementOverrides
_Check_return_ HRESULT
LoopingSelector::MeasureOverrideImpl(wf::Size availableSize, _Out_ wf::Size *returnValue)
{
    INT32 itemWidth = 0;

    LSTRACE(L"[%d] Measure called.", (((INT32)this) >> 8) & 0xFF);

    IFC_RETURN(LoopingSelectorGenerated::MeasureOverrideImpl(availableSize, returnValue));

    IFC_RETURN(get_ItemWidth(&itemWidth));

    if (itemWidth == 0 && availableSize.Width != std::numeric_limits<float>::infinity())
    {
        // If there is no itemWidth set, we use all the available space.
        returnValue->Width = availableSize.Width;
    }
    else
    {
        // If we have an itemWidth, use that for the desired size.
        returnValue->Width = static_cast<FLOAT>(itemWidth);
    }

    return S_OK;
}

_Check_return_ HRESULT
LoopingSelector::ArrangeOverrideImpl(wf::Size finalSize, _Out_opt_ wf::Size* returnValue)
{
    INT32 itemWidth = 0;
    DOUBLE verticalOffsetBeforeArrangeImpl = 0.0;
    bool expectedOffsetChange = false;
    FLOAT widthToReturn = 0.0;

    LSTRACE(L"[%d] Arrange called.", (((INT32)this) >> 8) & 0xFF);

    _isWithinArrangeOverride = TRUE;

    auto guard = wil::scope_exit([this, &finalSize]()
    {
        _lastArrangeSizeHeight = finalSize.Height;
        _isWithinArrangeOverride = FALSE;
    });

    IFC_RETURN(get_ItemWidth(&itemWidth));

    if (itemWidth != 0)
    {
        // Override the width with that of the first item's
        // width. A Canvas doesn't wrap
        // content so we do this so the control sizes correctly.
        widthToReturn = static_cast<FLOAT>(itemWidth);
    }
    else
    {
        // If no itemWidth has been set, we use all the available space.
        widthToReturn = finalSize.Width;

        // We compute a new value for _itemWidthFallback
        DOUBLE newItemWidthFallback = finalSize.Width;
        if (_itemWidthFallback != newItemWidthFallback)
        {
            _itemWidthFallback = newItemWidthFallback;
            _isSized = FALSE;
        }
    }

    if (_delayScrollPositionY != -1.0)
    {
        IFC_RETURN(SetScrollPosition(_delayScrollPositionY, FALSE /* useAnimation */));
        // SetScrollPosition sets _skipNextBalance to true. This is to prevent the call to Balance from OnViewChanged from running.
        // But OnViewChanged guards against calling Balance with _isWithinArrangeOverride. So the _skipNextBalance flag will not get cleared.
        // We need to explicitly clear the flag here, so that the call to Balance towards the end of this function has an effect.
        // See bug MSFT: 4711432 for more details.
        _skipNextBalance = FALSE;

        _delayScrollPositionY = -1.0;
        expectedOffsetChange = true;
    }

    if (_isScrollViewerInitialized && !_isSetupPending)
    {
        ASSERT(_tpScrollViewer.Get());
        IFC_RETURN(_tpScrollViewer->get_VerticalOffset(&verticalOffsetBeforeArrangeImpl));
    }

    IFC_RETURN(LoopingSelectorGenerated::ArrangeOverrideImpl(finalSize, returnValue));

    if (finalSize.Height != _lastArrangeSizeHeight && _isScrollViewerInitialized && !_isSetupPending)
    {
        // Orientation must have changed or we got resized, what used to be the middle point has changed.
        // So we need to shift the items to restore the old middle point item.

        const DOUBLE oldPanelSize = _panelSize;
        DOUBLE verticalOffsetAfterArrangeImpl = 0.0;

        IFC_RETURN(_tpScrollViewer->get_VerticalOffset(&verticalOffsetAfterArrangeImpl));

        IFC_RETURN(SizePanel());

        const DOUBLE delta = (_panelSize - oldPanelSize) / 2;
        _realizedTop += delta;
        _realizedBottom += delta;

        IFC_RETURN(ShiftChildren(delta));

        if (verticalOffsetAfterArrangeImpl != verticalOffsetBeforeArrangeImpl && !expectedOffsetChange)
        {
            // When moving from a small viewport to a large viewport during an orientation change,
            // the viewport vertical offset might get coerced and change. If that's the case,
            // we defer the scroll operation to the next layout pass because it's too late by then.
            _delayScrollPositionY = verticalOffsetBeforeArrangeImpl;
            _skipNextArrange = TRUE;
        }
    }

    // Adding the first item on the first Arrange pass after the item has
    // been added to the visual tree (and scrollViewer is initialized) causes
    // a second Arrange pass to occur. We skip it as an optimization.
    if (_skipNextArrange && _isScrollViewerInitialized)
    {
        LSTRACE(L"[%d] Skipping balance during this arrange.", (((INT32)this) >> 8) & 0xFF);
        _skipNextArrange = FALSE;
    }
    else
    {
        // The ScrollViewer's extents aren't valid until the first
        // arrange pass has occurred.
        if (!_isScrollViewerInitialized)
        {
            _isScrollViewerInitialized = TRUE;
        }
        IFC_RETURN(Balance(FALSE /* isOnSnapPoint */ ));
    }


    if (returnValue)
    {
        returnValue->Width = widthToReturn;
    }

    return S_OK;
}

_Check_return_ HRESULT
LoopingSelector::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;
    wrl::ComPtr<xaml_controls::IContentControl> spScrollViewerAsCC;
    wrl::ComPtr<xaml::IDependencyObject> spScrollViewerAsDO;
    wrl::ComPtr<xaml::IDependencyObject> spUpButtonAsDO;
    wrl::ComPtr<xaml_controls::Primitives::IButtonBase> spUpButtonAsButtonBase;
    wrl::ComPtr<xaml::IDependencyObject> spDownButtonAsDO;
    wrl::ComPtr<xaml_controls::Primitives::IButtonBase> spDownButtonAsButtonBase;

    LSTRACE(L"[%d] OnApplyTemplate called.", (((INT32)this) >> 8) & 0xFF);

    if (_tpScrollViewer)
    {
        IFC(_tpScrollViewer->remove_ViewChanged(_viewChangedToken));
        IFC(_tpScrollViewer->remove_ViewChanging(_viewChangingToken));
    }

    if (_tpUpButton)
    {
        IFC(_tpUpButton->remove_Click(_upButtonClickedToken))
    }

    if (_tpDownButton)
    {
        IFC(_tpDownButton->remove_Click(_downButtonClickedToken))
    }

    if (_tpScrollViewerPrivate)
    {
        IFC(_tpScrollViewerPrivate->EnableOverpan());
    }

    _tpScrollViewer.Clear();
    _tpScrollViewerPrivate.Clear();
    _tpPanel.Clear();
    _tpUpButton.Clear();
    _tpDownButton.Clear();

    IFC(LoopingSelectorGenerated::OnApplyTemplateImpl());

    IFC(QueryInterface(__uuidof(xaml_controls::IControlProtected), &spControlProtected));

    IFC(spControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(c_upButtonTemplatePart).Get(),
        &spUpButtonAsDO));

    if (spUpButtonAsDO)
    {
        IGNOREHR(spUpButtonAsDO.As(&spUpButtonAsButtonBase));
        IFC(SetPtrValue(_tpUpButton, spUpButtonAsButtonBase.Get()));
    }

    IFC(spControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(c_downButtonTemplatePart).Get(),
        &spDownButtonAsDO));

    if (spDownButtonAsDO)
    {
        IGNOREHR(spDownButtonAsDO.As(&spDownButtonAsButtonBase));
        IFC(SetPtrValue(_tpDownButton, spDownButtonAsButtonBase.Get()));
    }

    IFC(spControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(c_scrollViewerTemplatePart).Get(),
        &spScrollViewerAsDO));

    if (spScrollViewerAsDO)
    {
        wrl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;
        wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spScrollViewerPrivate;

        // Try to cast to IScrollViewer. If failed
        // just allow to remain NULL.
        IGNOREHR(spScrollViewerAsDO.As(&spScrollViewer));
        IGNOREHR(spScrollViewerAsDO.As(&spScrollViewerPrivate));
        IGNOREHR(spScrollViewerAsDO.As(&spScrollViewerAsCC));

        IFC(SetPtrValue(_tpScrollViewer, spScrollViewer.Get()));
        IFC(SetPtrValue(_tpScrollViewerPrivate, spScrollViewerPrivate.Get()));
    }

    if (spScrollViewerAsCC)
    {
        wrl::ComPtr<xaml_primitives::ILoopingSelectorPanel> spPanel;
        wrl::ComPtr<IInspectable> spLoopingSelectorPanelAsInspectable;
        IFC(wrl::MakeAndInitialize<LoopingSelectorPanel>(&spPanel));
        IFC(spPanel.As(&spLoopingSelectorPanelAsInspectable));
        IFC(spScrollViewerAsCC->put_Content(spLoopingSelectorPanelAsInspectable.Get()));
        IFC(SetPtrValue(_tpPanel, spPanel.Get()));
    }

    if (_tpPanel)
    {
        wrl::ComPtr<xaml::IFrameworkElement> spPanelAsFE;
        IFC(_tpPanel.As(&spPanelAsFE));
        IFC(spPanelAsFE->put_Height(1000000));
    }

    if (_tpUpButton)
    {
        IFC(_tpUpButton->add_Click(
            wrl::Callback<xaml::IRoutedEventHandler>(this, &LoopingSelector::OnUpButtonClicked).Get(),
            &_upButtonClickedToken));
    }

    if (_tpDownButton)
    {
        IFC(_tpDownButton->add_Click(
            wrl::Callback<xaml::IRoutedEventHandler>(this, &LoopingSelector::OnDownButtonClicked).Get(),
            &_downButtonClickedToken));
    }

    if (_tpScrollViewer)
    {
        IFC(_tpScrollViewer->add_ViewChanged(
            wrl::Callback<wf::IEventHandler<xaml_controls::ScrollViewerViewChangedEventArgs*>>
            (this, &LoopingSelector::OnViewChanged).Get(),
            &_viewChangedToken));

        IFC(_tpScrollViewer->add_ViewChanging(
            wrl::Callback<wf::IEventHandler<xaml_controls::ScrollViewerViewChangingEventArgs*>>
            (this, &LoopingSelector::OnViewChanging).Get(),
            &_viewChangingToken));
    }

    if (_tpScrollViewerPrivate)
    {
        IFC(_tpScrollViewerPrivate->DisableOverpan());
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region IUIElementOverrides

_Check_return_ HRESULT
LoopingSelector::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<LoopingSelector> spThis(this);
    wrl::ComPtr<ILoopingSelector> spThisAsILoopingSelector;
    wrl::ComPtr<xaml_automation_peers::LoopingSelectorAutomationPeer> spLoopingSelectorAutomationPeer;

    IFC(spThis.As(&spThisAsILoopingSelector));
    IFC(wrl::MakeAndInitialize<xaml_automation_peers::LoopingSelectorAutomationPeer>
            (&spLoopingSelectorAutomationPeer, spThisAsILoopingSelector.Get()));

    IFC(spLoopingSelectorAutomationPeer.CopyTo(returnValue));

    IFC(spLoopingSelectorAutomationPeer.AsWeak(&_wrAP));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

_Check_return_ HRESULT LoopingSelector::OnUpButtonClicked(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);

    return SelectPreviousItem();
}

_Check_return_ HRESULT LoopingSelector::OnDownButtonClicked(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);

    return SelectNextItem();
}

_Check_return_ HRESULT LoopingSelector::OnKeyDownImpl(
    _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers nModifierKeys;

    IFCPTR(pEventArgs);

    IFC(pEventArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(pEventArgs->get_Key(&key));

    IFC(PlatformHelpers::GetKeyboardModifiers(&nModifierKeys));
    if (!(nModifierKeys & wsy::VirtualKeyModifiers_Menu))
    {
        bHandled = TRUE;

        switch (key)
        {
        case wsy::VirtualKey_Up:
            IFC(SelectPreviousItem());
            break;
        case wsy::VirtualKey_Down:
            IFC(SelectNextItem());
            break;
        case wsy::VirtualKey_GamepadLeftTrigger:
        case wsy::VirtualKey_PageUp:
            IFC(HandlePageUpKeyPress());
            break;
        case wsy::VirtualKey_GamepadRightTrigger:
        case wsy::VirtualKey_PageDown:
            IFC(HandlePageDownKeyPress());
            break;
        case wsy::VirtualKey_Home:
            IFC(HandleHomeKeyPress());
            break;
        case wsy::VirtualKey_End:
            IFC(HandleEndKeyPress());
            break;
        default:
            bHandled = FALSE;
        }

        IFC(pEventArgs->put_Handled(bHandled));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT LoopingSelector::SelectNextItem()
{
    INT32 index;
    boolean shouldLoop = FALSE;

    IFC_RETURN(get_ShouldLoop(&shouldLoop));
    IFC_RETURN(get_SelectedIndex(&index));

    //Don't scroll past the end of the list if we are not looping:
    if (index < (INT32)_itemCount - 1 || shouldLoop)
    {
        DOUBLE pixelsToMove = _scaledItemHeight;
        IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, TRUE /* use animation */));
        IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::SelectPreviousItem()
{
    INT32 index;
    boolean shouldLoop = FALSE;

    IFC_RETURN(get_ShouldLoop(&shouldLoop));
    IFC_RETURN(get_SelectedIndex(&index));

    //Don't scroll past the start of the list if we are not looping
    if (index > 0 || shouldLoop)
    {
        DOUBLE pixelsToMove = -1 * _scaledItemHeight;
        IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, TRUE /* use animation */));
        IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::HandlePageDownKeyPress()
{
    DOUBLE viewportHeight = _unpaddedExtentBottom - _unpaddedExtentTop;
    DOUBLE pixelsToMove = viewportHeight / 2;
    IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, TRUE /* use animation */));
    IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::HandlePageUpKeyPress()
{
    DOUBLE viewportHeight = _unpaddedExtentBottom - _unpaddedExtentTop;
    DOUBLE pixelsToMove = -1 * viewportHeight / 2;
    IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, TRUE /* use animation */));
    IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::HandleHomeKeyPress()
{
    DOUBLE pixelsToMove = -1 * _realizedMidpointIdx * _scaledItemHeight;
    IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE /* use animation */));
    IFC_RETURN(Balance(TRUE /* isOnSnapPoint */));
    IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::HandleEndKeyPress()
{
    INT idxMovement = (_itemCount -1) - _realizedMidpointIdx;
    DOUBLE pixelsToMove = idxMovement * _scaledItemHeight;
    IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE /* use animation */));
    IFC_RETURN(Balance(TRUE /* isOnSnapPoint */));
    IFC_RETURN(RequestInteractionSound(xaml::ElementSoundKind_Focus));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::OnViewChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    HRESULT hr = S_OK;

    // In the new ScrollViewer2 interface OnViewChanged will actually
    // fire for all the intermediate ViewChanging events as well.
    // We only capture the final view.
    boolean isIntermediate = FALSE;
    pEventArgs->get_IsIntermediate(&isIntermediate);
    if (!isIntermediate)
    {
        LSTRACE(L"[%d] OnViewChanged called.", (((INT32)this) >> 8) & 0xFF);

        if (!_isWithinScrollChange && !_isWithinArrangeOverride)
        {
            IFC(Balance(TRUE /* isOnSnapPoint */ ));

            if (_itemState == ItemState::ManipulationInProgress)
            {
                IFC(TransitionItemsState(ItemState::Expanded));
                _itemState = ItemState::Expanded;
            }
            else if (_itemState == ItemState::LostFocus)
            {
                IFC(ExpandIfNecessary());
            }

            _skipSelectionChangeUntilFinalViewChanged = false;
        }
    }
    else
    {
        LSTRACE(L"[%d] Skipping ViewChanged event, within scroll", (((INT32)this) >> 8) & 0xFF);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::OnViewChanging(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::IScrollViewerViewChangingEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);
    HRESULT hr = S_OK;

    LSTRACE(L"[%d] OnViewChanging called.", (((INT32)this) >> 8) & 0xFF);

    if (!_isWithinScrollChange && !_isWithinArrangeOverride)
    {
        IFC(Balance(FALSE /* isOnSnapPoint */ ));

        // The Focus transition doesn't occur sometimes when flicking
        // on one ScrollViewer, than another before the first flick completes.
        // This is a possible ScrollViewer bug. Any time a manipulation
        // occurs we force focus.
        if (_itemState != ItemState::LostFocus && !_hasFocus)
        {
            wrl::ComPtr<xaml::IUIElement> spThisAsElement;
            BOOLEAN didSucceed = FALSE;
            xaml::FocusState focusState = xaml::FocusState::FocusState_Unfocused;
            IFC(QueryInterface(__uuidof(xaml::IUIElement), &spThisAsElement));
            IFC(spThisAsElement->get_FocusState(&focusState));

            if (focusState == xaml::FocusState_Unfocused)
            {
                IFC(spThisAsElement->Focus(xaml::FocusState::FocusState_Programmatic, &didSucceed));
            }
        }

        // During a manipulation no item should be highlighted.
        if (_itemState == ItemState::Expanded)
        {
            IFC(TransitionItemsState(ItemState::ManipulationInProgress));
            _itemState = ItemState::ManipulationInProgress;
            IFC(AutomationRaiseExpandCollapse());
        }
    }
    else
    {
        LSTRACE(L"[%d] Skipping ViewChanging event, within scroll", (((INT32)this) >> 8) & 0xFF);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::OnPointerEntered(
    _In_ IInspectable* pSender,
    _In_ xaml::Input::IPointerRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;
    wrl::ComPtr<ixp::IPointerPoint> spPointerPoint;

    IFC(pEventArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

    if (nPointerDeviceType != mui::PointerDeviceType_Touch)
    {
        IFC(GoToState(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"PointerOver")).Get(), FALSE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::OnPointerExited(
    _In_ IInspectable* pSender,
    _In_ xaml::Input::IPointerRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);

    IFC(GoToState(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Normal")).Get(), FALSE));
    IFC(TransitionItemsState(_itemState));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::GoToState(_In_ HSTRING strState, _In_ BOOLEAN useTransitions)
{
    wrl::ComPtr<xaml::IVisualStateManagerStatics> spVSMStatics;
    wrl::ComPtr<xaml_controls::IControl> spThisAsControl;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
        &spVSMStatics));

    IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IControl), &spThisAsControl));

    boolean returnValue = false;
    IFC_RETURN(spVSMStatics->GoToState(spThisAsControl.Get(), strState, useTransitions, &returnValue));

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::OnPressed(
    _In_ IInspectable* pSender,
    _In_ xaml::Input::IPointerRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);

    LSTRACE(L"[%d] OnPressed called.", (((INT32)this) >> 8) & 0xFF);

    if (_itemState == ItemState::LostFocus)
    {
        // LostFocus is only entered during a manipulation. Tapping
        // the control after LostFocus will reactivate it.
        _itemState = ItemState::ManipulationInProgress;
    }

    IFC(pEventArgs->put_Handled(TRUE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::OnLostFocus(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pEventArgs);

    BOOLEAN hasFocus = FALSE;
    IFC(HasFocus(&hasFocus));

    // We check to ensure that we previously did have focus
    // (stored in _hasFocus) but now don't (the result of hasFocus)
    // to ensure this is actually a focus transition.
    if (!hasFocus && _hasFocus)
    {
        _hasFocus = FALSE;
        if (_itemState == ItemState::ManipulationInProgress)
        {
            // If we're in the middle of a manipulation we
            // set itemState to LostFocus so completion of the
            // manipulation will collapse.
            _itemState = ItemState::LostFocus;
        }
        else
        {
            IFC(ExpandIfNecessary());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::OnGotFocus(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pEventArgs*/)
{
    BOOLEAN hasFocus = FALSE;
    IFC_RETURN(HasFocus(&hasFocus));

    if (hasFocus)
    {
        _hasFocus = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::OnItemTapped(
    _In_ IInspectable* pSender,
    _In_ xaml_input::ITappedRoutedEventArgs*)
{
    if (_itemState == ItemState::Expanded)
    {
        wrl::ComPtr<ILoopingSelectorItem> spLsiInterface;
        IFC_RETURN(pSender->QueryInterface<ILoopingSelectorItem>(&spLsiInterface));

        LoopingSelectorItem* lsiPtr = static_cast<LoopingSelectorItem*>(spLsiInterface.Get());

        INT itemVisualIndex = 0;
        UINT32 itemIndex = 0;
        IFC_RETURN(lsiPtr->GetVisualIndex(&itemVisualIndex));
        IFC_RETURN(VisualIndexToItemIndex(itemVisualIndex, &itemIndex));

        // We need to make sure that we check against the selected index,
        // not the midpoint index.  Normally, the item in the center of the view
        // is also the selected item, but in the case of UI automation, it
        // is not always guaranteed to be.
        INT32 selectedIndex = 0;
        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        if (itemIndex == static_cast<UINT32>(selectedIndex))
        {
            IFC_RETURN(ExpandIfNecessary());
        }
        else
        {
            // Tapping any other time scrolls to that item.
            DOUBLE pixelsToMove = static_cast<double>(itemVisualIndex - _realizedMidpointIdx) * (_scaledItemHeight);
            IFC_RETURN(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, TRUE /* use animation */));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::OnPropertyChanged(
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyProperty> spPropertyInfo;

    IFC(pArgs->get_Property(&spPropertyInfo));

    if (spPropertyInfo.Get() == LoopingSelectorFactory::s_ItemsProperty)
    {
        LSTRACE(L"[%d] Items property refreshed.", (((INT32)this) >> 8) & 0xFF);

        // Optimization: When items change the size of the ScrollViewer does not.
        // We clear all items and allow for the balancing function to realize
        // and replace the items as instead of refreshing the entire thing.
        IFC(ClearAllItems());
        IFC(Balance(FALSE /* isOnSnapPoint */ ));

        IFC(AutomationClearPeerMap());

        IFC(AutomationRaiseSelectionChanged());
    }
    else if (spPropertyInfo.Get() == LoopingSelectorFactory::s_ShouldLoopProperty)
    {
        LSTRACE(L"[%d] ShouldLoop property refreshed.", (((INT32)this) >> 8) & 0xFF);

        // ShouldLoop will require resizing the bounds of the ScrollViewer, as a result
        // we start from scratch and re-setup everything.
        IFC(ClearAllItems());
        _isSized = FALSE;
        _isSetupPending = TRUE;
        IFC(Balance(FALSE /* isOnSnapPoint */ ));
    }
    else if (spPropertyInfo.Get() == LoopingSelectorFactory::s_SelectedIndexProperty && !_disablePropertyChange)
    {
        LSTRACE(L"[%d] Selected Index property refreshed.", (((INT32)this) >> 8) & 0xFF);
        wrl::ComPtr<IInspectable> spValue;
        wrl::ComPtr<wf::IReference<INT32>> spReferenceInt;
        INT32 newIdx = 0;
        INT32 oldIdx = 0;

        IFC(pArgs->get_NewValue(&spValue));
        IFC(spValue.As(&spReferenceInt));
        IFC(spReferenceInt->get_Value(&newIdx));

        IFC(pArgs->get_OldValue(&spValue));
        IFC(spValue.As(&spReferenceInt));
        IFC(spReferenceInt->get_Value(&oldIdx));

        // NOTE: This call only will be applied when the
        // ScrollViewer is not being manipulated. Once a
        // manipulation has begun the user's eventual
        // selection takes dominance.
        IFC(SetSelectedIndex(oldIdx, newIdx));

        IFC(Balance(FALSE /* isOnSnapPoint */ ));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::IsTemplateAndItemsAttached(_Out_ BOOLEAN* result)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wfc::IVector<IInspectable *>> spItems;

    IFCPTR(result);
    *result = FALSE;

    if (_tpScrollViewer &&
        _tpPanel)
    {
        IFC(get_Items(spItems.GetAddressOf()));

        if (spItems)
        {
            UINT32 itemCount = 0;
            IFC(spItems->get_Size(&itemCount));
            _itemCount = itemCount;
            if (itemCount > 0)
            {
                *result = TRUE;
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::IsSetupForAutomation(_Out_ BOOLEAN* isSetup)
{
    HRESULT hr = S_OK;

    BOOLEAN isTemplateAndItemsAttached = FALSE;
    IFC(IsTemplateAndItemsAttached(&isTemplateAndItemsAttached));
    *isSetup = isTemplateAndItemsAttached && _isSized && !_isSetupPending;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::Balance(_In_ BOOLEAN isOnSnapPoint)
{
    HRESULT hr = S_OK;

    BOOLEAN isTemplateAndItemsAttached = FALSE;
    BOOLEAN abortBalance = FALSE;

    IFC(IsTemplateAndItemsAttached(&isTemplateAndItemsAttached));
    abortBalance = !isTemplateAndItemsAttached;

    // Normalize will call SetScrollPosition, which
    // triggers another ViewChanged event to occur.
    // We skip the balance called from ViewChanged.
    if (!abortBalance && _skipNextBalance)
    {
        _skipNextBalance = FALSE;
        LSTRACE(L"[%d] Skipping balance.", (((INT32)this) >> 8) & 0xFF);
        abortBalance = TRUE;
    }

    if (!abortBalance)
    {
        IFC(MeasureExtent(&_unpaddedExtentTop, &_unpaddedExtentBottom));

        // 100000 is a arbitrary number that is larger than any possible screen size.
        // If the visible area of the ScrollViewer is larger than this value then
        // it's likely the control is placed in a ScrollViewer or other layout element
        // that feeds infinite size to its children. In these situations we avoid
        // balancing.
        if (_unpaddedExtentBottom - _unpaddedExtentTop > 100000)
        {
            _skipNextArrange = FALSE;
            LSTRACE(L"[%d] Skipping balance. Extents too large, LoopingSelector is not designed to operate with infinite or very large extents.", (((INT32)this) >> 8) & 0xFF);
            abortBalance = TRUE;
        }
    }

    if (!abortBalance)
    {
        IFC(EnsureSetup());
        abortBalance = !_isSized || _isSetupPending;
    }

    if (!abortBalance)
    {
        UINT32 headAdd = 0;
        UINT32 headTrim = 0;
        UINT32 tailAdd = 0;
        UINT32 tailTrim = 0;

        INT32 maxTopIdx = 0;
        INT32 maxBottomIdx = 0;
        double paddedExtentTop = 0.0;
        double paddedExtentBottom = 0.0;
        double viewportHeight = 0.0;

        IFC(GetMaximumAddIndexPosition(&maxTopIdx, &maxBottomIdx));

        // We only normalize on snap points because otherwise a manipulation
        // is in progress and SetScrollPosition will
        // coerce the scroll offset onto a snap point, even if we're currently
        // between two.
        if (isOnSnapPoint)
        {
            IFC(Normalize());
        }

        viewportHeight = _unpaddedExtentBottom - _unpaddedExtentTop;
        // Virtualization buffer is a half viewport in either direction.
        paddedExtentTop = _unpaddedExtentTop - viewportHeight/2;
        paddedExtentBottom = _unpaddedExtentBottom + viewportHeight/2;

        // By adding a pixel we assure we never trim and re-add an element.
        while (_realizedTop < (paddedExtentTop + 1.0 - _scaledItemHeight) && _realizedItems.size() > 0)
        {
           IFC(Trim(ListEnd::Head));
           headTrim++;
        }

        while (_realizedBottom > (paddedExtentBottom - 1.0 + _scaledItemHeight) && _realizedItems.size() > 0)
        {
            IFC(Trim(ListEnd::Tail));
            tailTrim++;
        }

        while (_realizedTop > paddedExtentTop && _realizedTopIdx > maxTopIdx)
        {
            IFC(Add(ListEnd::Head));
            headAdd++;
        }

        while (_realizedBottom < paddedExtentBottom && _realizedBottomIdx < maxBottomIdx)
        {
            IFC(Add(ListEnd::Tail));
            tailAdd++;
        }

        if (headAdd > 0 || tailAdd > 0 || tailTrim > 0 || tailAdd > 0)
        {
            IFC(AutomationRaiseStructureChanged());
        }

        LSTRACE(L"[%d] HeadAdd: %d HeadTrim: %d TailAdd: %d TailTrim: %d Realized: %d Recycled: %d", (((INT32)this) >> 8) & 0xFF, headAdd, headTrim, tailAdd, tailTrim, _realizedItems.size(), _recycledItems.size());

        // During manipulation we don't update the selected index. Only when
        // we reach a final value.
        if (isOnSnapPoint || _itemState == ItemState::Expanded)
        {
            IFC(UpdateSelectedItem());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::Normalize()
{
    HRESULT hr = S_OK;
    boolean shouldLoop = FALSE;

    IFC(get_ShouldLoop(&shouldLoop));
    // Only normalize if strayed 50px+ from center and if
    // in looping mode. Non-looping selectors don't normalize.
    if (shouldLoop && fabs(_unpaddedExtentTop - _panelMidpointScrollPosition) > 50.0)
    {
        double delta = _panelMidpointScrollPosition - _unpaddedExtentTop;

        // WORKAROUND: It's likely there's a bug in DManip that causes it to
        // end a manipulation not on a snap point when two fingers are on the
        // ScrollViewer. We explicitly make sure we are on a snap point here.
        // Delaying bug filing until our input system is more final.
        BOOLEAN isActuallyOnSnapPoint = (fabs(delta / _scaledItemHeight - floor(delta / _scaledItemHeight)) < 0.001);

        if (isActuallyOnSnapPoint)
        {
            _realizedTop += delta;
            _realizedBottom += delta;

            // These are adjusted for the duration of the balance. ScrollViewer
            // requires an invalidate pass which occurs before the next ViewChanged
            // event for its extents to become correct, we manually account
            // for the offset here since the next Balance happens before that.
            _unpaddedExtentTop += delta;
            _unpaddedExtentBottom += delta;

            IFC(ShiftChildren(delta));

            LSTRACE(L"[%d] Shifted %f (%f items) %d", (((INT32)this) >> 8) & 0xFF, delta, delta / _scaledItemHeight, isActuallyOnSnapPoint);

            IFC(SetScrollPosition(_panelMidpointScrollPosition, FALSE));

            // Skip the balance occurring after ViewChanged.
            _skipNextBalance = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::EnsureSetup()
{
    HRESULT hr = S_OK;
    INT32 selectedIdx = 0;

    IFC(get_SelectedIndex(&selectedIdx));

    if (!_isSized)
    {
        LSTRACE(L"[%d] Sizing panel.", (((INT32)this) >> 8) & 0xFF);
        INT32 itemDimAsInt = 0;
        // Optimization: caching itemHeight and itemWidth.
        IFC(get_ItemHeight(&itemDimAsInt));
        _itemHeight = static_cast<double>(itemDimAsInt);
        _scaledItemHeight = static_cast<DOUBLE>(itemDimAsInt);

        IFC(get_ItemWidth(&itemDimAsInt));

        if (itemDimAsInt == 0)
        {
            // If we don't have an explicitly set ItemWidth, we fallback to this value which is computed during Arrange.
            _itemWidth = _itemWidthFallback;
        }
        else
        {
            _itemWidth = static_cast<double>(itemDimAsInt);
        }

        IFC(ClearAllItems());
        IFC(SizePanel());

        _isSized = TRUE;
    }

    // ScrollViewer isn't fully initialized until the
    // first Arrange pass and will return invalid extents
    // and ignore SetScrollPosition requests.
    if (_isScrollViewerInitialized && _isSetupPending)
    {
        LSTRACE(L"[%d] Setting up bounds and scroll viewer.", (((INT32)this) >> 8) & 0xFF);
        double viewportHeight = 0.0;
        double verticalOffset = 0.0;
        double newScrollPosition = 0.0;
        double startPoint = 0.0;
        boolean shouldLoop = FALSE;

        IFC(get_ShouldLoop(&shouldLoop));

        IFC(_tpScrollViewer->get_ViewportHeight(&viewportHeight));
        IFC(_tpScrollViewer->get_VerticalOffset(&verticalOffset));
        IFC(SetupSnapPoints(0.0, _scaledItemHeight));

        // If in looping mode the selector is setup
        // in the middle of the scrollable area, if in non-looping
        // mode it is setup to be at the currently selected item's offset.
        // (Non-looping mode sizes the scrollable area to be precisely large
        //  enough for the item count)
        if (shouldLoop)
        {
            startPoint = _panelSize / 2;
            _panelMidpointScrollPosition = startPoint - viewportHeight / 2 + _scaledItemHeight / 2;

            _realizedTop = startPoint;
            _realizedBottom = startPoint;
            newScrollPosition = _panelMidpointScrollPosition;
        }
        else
        {
            startPoint = (_panelSize - (_itemCount) * _scaledItemHeight) / 2;
            _panelMidpointScrollPosition = startPoint - viewportHeight / 2 + _scaledItemHeight / 2;
            _realizedTop = startPoint + selectedIdx * _scaledItemHeight;
            _realizedBottom = startPoint + selectedIdx * _scaledItemHeight;
            newScrollPosition = _panelMidpointScrollPosition + selectedIdx * _scaledItemHeight;
        }

        _realizedTopIdx = selectedIdx;
        _realizedBottomIdx = selectedIdx -1;

        // Optimization: skip scrolling if ScrollViewer is
        // in correct position (often TRUE after normalization)
        if (fabs(verticalOffset - newScrollPosition) > 1.0)
        {
            IFC(SetScrollPosition(newScrollPosition, FALSE));
            _unpaddedExtentTop += newScrollPosition - verticalOffset;
            _unpaddedExtentBottom += newScrollPosition - verticalOffset;

            // Skip the balance caused by the ViewChange event fired
            // from SetScrollPosition.
            _skipNextBalance = TRUE;
        }

        _isSetupPending = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::SetSelectedIndex(_In_ INT32 oldIdx, _In_ INT32 newIdx)
{
    HRESULT hr = S_OK;

    double pixelsToMove = 0.0;
    BOOLEAN isTemplateAndItemsAttached = FALSE;

    IFC(IsTemplateAndItemsAttached(&isTemplateAndItemsAttached));

    // Only set the new index position if we're in the idle position
    // and the control is properly initialized and the oldIndex is meaningful.
    if (oldIdx != -1 && isTemplateAndItemsAttached && _itemState == ItemState::Expanded)
    {
        LSTRACE(L"[%d] SetSelectedIndex From %d To %d called.", (((INT32)this) >> 8) & 0xFF, oldIdx, newIdx);
        pixelsToMove = static_cast<double>(newIdx - oldIdx) * (_scaledItemHeight);
        IFC(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::Trim(_In_ ListEnd end)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<ILoopingSelectorItem> spChildAsLSI;

    if (_realizedItems.empty())
    {
        LSTRACE(L"[%d] Trim called with empty list.", (((INT32)this) >> 8) & 0xFF);
        goto Cleanup;
    }

    if (end == ListEnd::Head)
    {
        //COMPtr assignment causes AddRef.
        spChildAsLSI = _realizedItems.back();
        _realizedItems.pop_back();
    }
    else
    {
        spChildAsLSI = _realizedItems.front();
        _realizedItems.erase(_realizedItems.begin());
    }

    if (end == ListEnd::Head)
    {
        _realizedTop += _scaledItemHeight;
        _realizedTopIdx++;
    }
    else
    {
        _realizedBottom -= _scaledItemHeight;
        _realizedBottomIdx--;
    }

    IFC(RecycleItem(spChildAsLSI.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::Add(_In_ ListEnd end)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<ILoopingSelectorItem> spChild;
    wrl::ComPtr<xaml::IUIElement> spChildAsUI;

    if (end == ListEnd::Head)
    {
        IFC(RealizeItem(_realizedTopIdx - 1, spChild.GetAddressOf()));
        // Panel's Children keeps the reference to this item.
        _realizedItems.push_back(spChild.Get());
        IFC(spChild.As(&spChildAsUI));
        IFC(SetPosition(spChildAsUI.Get(), _realizedTop - _scaledItemHeight));
        _realizedTop -= _scaledItemHeight;
        _realizedTopIdx--;
    }
    else
    {
        IFC(RealizeItem(_realizedBottomIdx + 1, spChild.GetAddressOf()));
        // Panel's Children keeps the reference to this item.
        _realizedItems.insert(_realizedItems.begin(), spChild.Get());
        IFC(spChild.As(&spChildAsUI));
        IFC(SetPosition(spChildAsUI.Get(), _realizedBottom));
        _realizedBottom += _scaledItemHeight;
        _realizedBottomIdx++;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::GetMaximumAddIndexPosition(_Out_ INT32* headIdx, _Out_ INT32* tailIdx)
{
    HRESULT hr = S_OK;

    boolean shouldLoop;
    IFC(get_ShouldLoop(&shouldLoop));

    if (shouldLoop)
    {
        *headIdx = (std::numeric_limits<INT32>::min)();
        *tailIdx = (std::numeric_limits<INT32>::max)();
    }
    else
    {
        *headIdx = 0;
        *tailIdx = _itemCount - 1;
    }

Cleanup:
    RRETURN(hr);
}

// In cases where we're directly setting the selected item (e.g., via UIA),
// we don't care whether we're in the middle of scrolling.
// We'll use ignoreScrollingState to flag such scenarios.
_Check_return_ HRESULT LoopingSelector::UpdateSelectedItem(bool ignoreScrollingState)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsCollection;
    wrl::ComPtr<IInspectable> spSelectedItem;
    wrl::ComPtr<IInspectable> spPreviouslySelectedItem;

    UINT32 itemCount = 0;
    double midpoint = 0.0;
    UINT32 newIdx = 0;
    INT32 oldIdx = 0;

    // This will be in the middle of the currently selected item.
    midpoint = (_unpaddedExtentTop + _unpaddedExtentBottom) / 2 - _realizedTop;
    newIdx = _realizedTopIdx +
        static_cast<UINT32>(midpoint) / static_cast<UINT32>(_scaledItemHeight);

    IFC(get_Items(&spItemsCollection));
    IFC(spItemsCollection->get_Size(&itemCount));

    // Normally, when an item is scrolled into the center of our view,
    // we want to automatically select that item.  However, in the case of
    // UI automation (e.g., Narrator), users will be iterating through the
    // looping selector items one at a time to hear them read out,
    // in order to find the one that they want to select.  In this case,
    // we don't want to automatically select the item that is scrolled into view,
    // so in that circumstance we skip selecting the new item.
    // However, we do still want to put the item in the *visual state*
    // of being selected, since it will be appearing in the middle,
    // meaning that we want the font color of the item in that position
    // to properly match the background.
    IFC(UpdateVisualSelectedItem(_realizedMidpointIdx, newIdx));

    _realizedMidpointIdx = newIdx;

    if (ignoreScrollingState || !_skipSelectionChangeUntilFinalViewChanged)
    {
        newIdx = PositiveMod(newIdx, itemCount);

        IFC(spItemsCollection->GetAt(newIdx, &spSelectedItem));

        _disablePropertyChange = TRUE;
        IFC(get_SelectedIndex(&oldIdx));
        IFC(put_SelectedIndex(static_cast<INT32>(newIdx)));
        IFC(get_SelectedItem(&spPreviouslySelectedItem));

        IFC(put_SelectedItem(spSelectedItem.Get()));

        if ((UINT32)oldIdx != newIdx)
        {
            IFC(RaiseOnSelectionChanged(spPreviouslySelectedItem.Get(), spSelectedItem.Get()));
            IFC(AutomationRaiseSelectionChanged());
        }

        _disablePropertyChange = FALSE;
    }

Cleanup:
    RRETURN(hr);
}


// NOTE: Only called when the ScrollViewer is done Running (e.g. no scrolling is happening).
_Check_return_ HRESULT LoopingSelector::UpdateVisualSelectedItem(_In_ UINT32 oldIdx, _In_ UINT32 newIdx)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<ILoopingSelectorItem> spEltAsLSI;
    LoopingSelectorItem* lsi = nullptr;

    if (_realizedItems.size() > 0)
    {
        if (_realizedItems.size() > oldIdx - _realizedTopIdx)
        {
            spEltAsLSI = _realizedItems[_realizedItems.size() - (oldIdx - _realizedTopIdx) - 1];
            lsi = static_cast<LoopingSelectorItem*>(spEltAsLSI.Get());
            if (_itemState == ItemState::Expanded)
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Expanded, TRUE));
            }
            else
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Normal, TRUE));
            }
        }

        if (_realizedItems.size() > newIdx - _realizedTopIdx)
        {
            spEltAsLSI = _realizedItems[_realizedItems.size() - (newIdx - _realizedTopIdx) - 1];
            lsi = static_cast<LoopingSelectorItem*>(spEltAsLSI.Get());
            IFC(lsi->SetState(LoopingSelectorItem::State::Selected, TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::VisualIndexToItemIndex(_In_ UINT32 visualIndex, _Out_ UINT32* itemIndex)
{
    IFCPTR_RETURN(itemIndex);

    wrl::ComPtr<wfc::IVector<IInspectable*>> itemsCollection;
    IFC_RETURN(get_Items(&itemsCollection));

    UINT32 itemCount = 0;
    IFC_RETURN(itemsCollection->get_Size(&itemCount));

    *itemIndex = PositiveMod(visualIndex, itemCount);

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::RealizeItem(_In_ UINT32 itemIdxToRealize, _Outptr_ ILoopingSelectorItem** ppItem)
{
    wrl::ComPtr<ILoopingSelectorItem> spLoopingSelectorItem;
    wrl::ComPtr<xaml_controls::IContentControl> spLoopingSelectorItemAsCC;
    wrl::ComPtr<xaml::IDependencyObject> spLoopingSelectorItemAsDO;
    LoopingSelectorItem* lsi = nullptr;

    IFCPTR_RETURN(ppItem);

    UINT32 moddedItemIdx = 0;
    IFC_RETURN(VisualIndexToItemIndex(itemIdxToRealize, &moddedItemIdx));

    bool wasItemRecycled = false;

    IFC_RETURN(RetreiveItemFromAPRealizedItems(moddedItemIdx, &spLoopingSelectorItem));
    if (!spLoopingSelectorItem && _recycledItems.size() != 0)
    {
        spLoopingSelectorItem = _recycledItems.back();
        wasItemRecycled = true;
    }

    if (!spLoopingSelectorItem)
    {
        wrl::ComPtr<xaml::IUIElement> spLSIAsUIElt;
        wrl::ComPtr<xaml::IFrameworkElement> spLSIAsFE;
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spPanelChildren;
        wrl::ComPtr<xaml::IDataTemplate> spDataTemplate;
        wrl::ComPtr<xaml_controls::IControl> spLSIAsControl;
        wrl::ComPtr<xaml_controls::IControl> spThisAsControl;
        EventRegistrationToken tappedToken = {};
        BOOLEAN visualTreeRebuilt = FALSE;

        IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IControl), &spThisAsControl));

        UINT32 childCount;
        IFC_RETURN(GetPanelChildren(&spPanelChildren, &childCount));

        IFC_RETURN(wrl::MakeAndInitialize<LoopingSelectorItem>(&spLoopingSelectorItem));
        IFC_RETURN(spLoopingSelectorItem.As(&spLSIAsUIElt));
        IFC_RETURN(spLoopingSelectorItem.As(&spLSIAsControl));
        IFC_RETURN(spLoopingSelectorItem.As(&spLoopingSelectorItemAsCC));
        IFC_RETURN(spLoopingSelectorItem.As(&spLoopingSelectorItemAsDO));
        IFC_RETURN(spLoopingSelectorItem.As(&spLSIAsFE));
        lsi = static_cast<LoopingSelectorItem*>(spLoopingSelectorItem.Get());

        IFC_RETURN(lsi->SetParent(this));
        IFC_RETURN(get_ItemTemplate(&spDataTemplate));
        IFC_RETURN(spLoopingSelectorItemAsCC->put_ContentTemplate(spDataTemplate.Get()));
        IFC_RETURN(spLSIAsFE->put_Width(_itemWidth));
        IFC_RETURN(spLSIAsFE->put_Height(_itemHeight));
        IFC_RETURN(spPanelChildren->Append(spLSIAsUIElt.Get()));

        xaml::HorizontalAlignment horizontalAlignment;
        xaml::Thickness padding;

        IFC_RETURN(spThisAsControl->get_HorizontalContentAlignment(&horizontalAlignment));
        IFC_RETURN(spLSIAsControl->put_HorizontalContentAlignment(horizontalAlignment));

        IFC_RETURN(spThisAsControl->get_Padding(&padding));
        IFC_RETURN(spLSIAsControl->put_Padding(padding));

        // The event will be disconnected when the item is destroyed. No
        // need to keep track of the token.
        IFC_RETURN(spLSIAsUIElt->add_Tapped(wrl::Callback<xaml::Input::ITappedEventHandler>
            (this, &LoopingSelector::OnItemTapped).Get(),
            &tappedToken));

        IFC_RETURN(spLSIAsControl->ApplyTemplate(&visualTreeRebuilt));
    }
    else
    {
        wrl::ComPtr<xaml::IFrameworkElement> spLSIAsFE;

        lsi = static_cast<LoopingSelectorItem*>(spLoopingSelectorItem.Get());

        if (wasItemRecycled)
        {
            _recycledItems.pop_back();
        }

        IFC_RETURN(spLoopingSelectorItem.As(&spLoopingSelectorItemAsCC));
        IFC_RETURN(spLoopingSelectorItem.As(&spLoopingSelectorItemAsDO));
        IFC_RETURN(spLoopingSelectorItem.As(&spLSIAsFE));
        IFC_RETURN(spLSIAsFE->put_Width(_itemWidth));
    }

    // Retrieve the data item and set it as content.
    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsCollection;
    IFC_RETURN(get_Items(&spItemsCollection));
    wrl::ComPtr<IInspectable> spItem;
    IFC_RETURN(spItemsCollection->GetAt(moddedItemIdx, &spItem));
    IFC_RETURN(spLoopingSelectorItemAsCC->put_Content(spItem.Get()));

    wrl::ComPtr<xaml_automation::IAutomationPropertiesStatics> spAutomationPropertiesStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
        &spAutomationPropertiesStatics));

    // To get the position in set, we add 1 to the item index - this is so Narrator announces
    // (e.g.) "1 of 30" for the item at index 0, since "0 of 30" through "29 of 30" would be
    // very unexpected to users.
    UINT32 itemCount = 0;
    IFC_RETURN(spItemsCollection->get_Size(&itemCount));
    IFC_RETURN(spAutomationPropertiesStatics->SetPositionInSet(spLoopingSelectorItemAsDO.Get(), moddedItemIdx + 1));
    IFC_RETURN(spAutomationPropertiesStatics->SetSizeOfSet(spLoopingSelectorItemAsDO.Get(), itemCount));

    IFC_RETURN(lsi->SetVisualIndex(itemIdxToRealize));

    if (_itemState == ItemState::Expanded || _itemState == ItemState::ManipulationInProgress || _itemState == ItemState::LostFocus)
    {
        IFC_RETURN(lsi->SetState(LoopingSelectorItem::State::Expanded, FALSE));
    }
    else
    {
        IFC_RETURN(lsi->SetState(LoopingSelectorItem::State::Normal, FALSE));
    }

    IFC_RETURN(lsi->AutomationUpdatePeerIfExists(moddedItemIdx));
    IFC_RETURN(spLoopingSelectorItem.CopyTo(ppItem));

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelector::RecycleItem(_In_ ILoopingSelectorItem* pItem)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<ILoopingSelectorItem> spItemAsLSI;
    wrl::ComPtr<xaml::IUIElement> spItemAsUI;

    spItemAsLSI = pItem;
    spItemAsLSI.As(&spItemAsUI);

    _recycledItems.push_back(pItem);

    // Removing from the visual tree is expensive. Place offscreen instead.
    ASSERT(_spCanvasStatics);
    IFC(_spCanvasStatics->SetLeft(spItemAsUI.Get(), -10000));

Cleanup:
    RRETURN(hr);
}

#pragma region Helpers

_Check_return_ HRESULT LoopingSelector::HasFocus(_Out_ BOOLEAN* pHasFocus)
{
    wrl::ComPtr<IInspectable> spFocusedElt;
    wrl::ComPtr<xaml::IDependencyObject> spFocusedEltAsDO;

    *pHasFocus = FALSE;

    wrl::ComPtr<xaml::IUIElement> thisAsUIE;
    IFC_RETURN(this->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
    IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

    if (xamlRoot)
    {
        wrl::ComPtr<xaml_input::IFocusManagerStatics> spFocusManager;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
            &spFocusManager));

        IFC_RETURN(spFocusManager->GetFocusedElementWithRoot(xamlRoot.Get(), &spFocusedElt));

        if (spFocusedElt)
        {
            IFC_RETURN(spFocusedElt.As(&spFocusedEltAsDO));
            IFC_RETURN(IsAscendantOfTarget(spFocusedEltAsDO.Get(), pHasFocus));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::IsAscendantOfTarget(_In_ xaml::IDependencyObject* pChild, _Out_ BOOLEAN* pIsChildOfTarget)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IDependencyObject> spCurrentDO(pChild);
    wrl::ComPtr<xaml::IDependencyObject> spParentDO;
    wrl::ComPtr<xaml::IDependencyObject> spThisAsDO;
    wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVTHStatics;

    BOOLEAN isFound = FALSE;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
        &spVTHStatics));
    IFC(QueryInterface(__uuidof(xaml::IDependencyObject), &spThisAsDO));

    while (spCurrentDO && !isFound)
    {
        if (spCurrentDO.Get() == spThisAsDO.Get())
        {
            isFound = TRUE;
        }
        else
        {
            IFC(spVTHStatics->GetParent(spCurrentDO.Get(), spParentDO.ReleaseAndGetAddressOf()));
            spCurrentDO.Attach(spParentDO.Detach());
        }
    }

    *pIsChildOfTarget = isFound;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::ShiftChildren(_In_ double delta)
{
    HRESULT hr = S_OK;

    std::vector<ILoopingSelectorItem*>::iterator iter;

    for (iter = _realizedItems.begin(); iter != _realizedItems.end(); iter++)
    {
        wrl::ComPtr<ILoopingSelectorItem> spChild;
        wrl::ComPtr<xaml::IUIElement> spChildAsUI;
        double currentPosition = 0.0;
        // This keeps the ref count unchanged. Attach doesn't
        // AddRef, and Detach doesn't Release.
        spChild.Attach(*iter);
        IFC(spChild.As(&spChildAsUI));
        ASSERT(_spCanvasStatics);
        IFC(_spCanvasStatics->GetTop(spChildAsUI.Get(), &currentPosition));
        IFC(_spCanvasStatics->SetTop(spChildAsUI.Get(), currentPosition + delta));
        spChild.Detach();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::MeasureExtent(_Out_ double* extentTop, _Out_ double* extentBottom)
{
    HRESULT hr = S_OK;
    double viewportHeight = 0.0;
    double verticalOffset = 0.0;

    IFC(_tpScrollViewer->get_ViewportHeight(&viewportHeight));
    IFC(_tpScrollViewer->get_VerticalOffset(&verticalOffset));

    *extentTop = verticalOffset;
    *extentBottom = verticalOffset + viewportHeight;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::ClearAllItems()
{
    wrl::ComPtr<wfc::IVector<IInspectable *>> spItems;
    std::vector<ILoopingSelectorItem*>::iterator iter;

    for (iter = _realizedItems.begin(); iter != _realizedItems.end(); iter++)
    {
        IFC_RETURN(RecycleItem(*iter));
        _realizedBottom -= _scaledItemHeight;
        _realizedBottomIdx--;
    }
    _realizedItems.clear();
    _realizedItemsForAP.clear();

    IFC_RETURN(get_Items(spItems.GetAddressOf()));
    if (spItems)
    {
        // We reset the logical indices to not contain any extra multiples of
        // the item count. This makes scenarios where the items collection is
        // changed while the user is manipulating the control do the 'expected'
        // thing and not jump around.
        UINT itemCount = 0;
        INT indexDelta = 0;
        IFC_RETURN(spItems->get_Size(&itemCount));
        _itemCount = itemCount;
        indexDelta = _realizedMidpointIdx - PositiveMod(_realizedMidpointIdx, _itemCount);

        _realizedMidpointIdx -= indexDelta;
        _realizedTopIdx -= indexDelta;
        _realizedBottomIdx -= indexDelta;
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelector::GetPanelChildren(_Outptr_ wfc::IVector<xaml::UIElement*>** ppChildren, _Out_ UINT32* count)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IPanel> spPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(_tpPanel.As(&spPanel));
    IFC(spPanel->get_Children(&spChildren));
    IFC(spChildren->get_Size(count));

    *ppChildren = spChildren.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::SizePanel()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IFrameworkElement> spPanelAsFE;
    wrl::ComPtr<xaml::IUIElement> spThisAsUI;

    boolean shouldLoop = FALSE;
    IFC(get_ShouldLoop(&shouldLoop));
    IFC(QueryInterface(__uuidof(xaml::IUIElement), &spThisAsUI));
    IFC(_tpPanel.As(&spPanelAsFE));

    if (shouldLoop)
    {
        double scrollViewerHeight = 0.0;
        IFC(_tpScrollViewer->get_ViewportHeight(&scrollViewerHeight));

        // This is a large number. It is large enough to ensure for any
        // item size the panel size exceeds that which is reasonable
        // to expect the user to flick to the end of continuously while
        // not allowing a manipulation to complete.
        //
        // It is odd so the panel sizes correctly. The
        // midpoint aligns with the snap points and visual item realization
        // position.
        _panelSize = scrollViewerHeight + (1001) * _scaledItemHeight;
    }
    else
    {
        double scrollViewerHeight = 0.0;

        IFC(_tpScrollViewer->get_ViewportHeight(&scrollViewerHeight));
        _panelSize = scrollViewerHeight + (_itemCount - 1) * _scaledItemHeight;

        // WPB# 264945
        // on high resolution device, actual height will be rounded based
        // on the plateau scale factor, sometime the panel's actual height
        // is less than given height. In this case the items can't fit in
        // Panel and cause items shifting up a bit. This will trigger a
        // view changing event and the items are expanded when
        // LoopingSelector is loaded.
        // So here we should give panel 1 more pixel to make sure items
        // can fill in the panel.
        _panelSize += 1.0;
    }

    IFC(spPanelAsFE->put_Height(static_cast<double>(_panelSize)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::SetScrollPosition(_In_ double offset, _In_ BOOLEAN useAnimation)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spVerticalOffsetAsInspectable;
    wrl::ComPtr<wf::IReference<double>> spVerticalOffset;

    IFC(Private::ValueBoxer::CreateDouble(offset, &spVerticalOffsetAsInspectable));
    IFC(spVerticalOffsetAsInspectable.As(&spVerticalOffset));

    LSTRACE(L"[%d] Setting scroll position %f", (((INT32)this) >> 8) & 0xFF, offset);


    _skipNextBalance = TRUE;

    if (!useAnimation)
    {
        BOOLEAN didSucceed = FALSE;
        // We use this boolean as a performance optimization. When
        // this function is called with useAnimation set to FALSE it
        // is an instantaneous jump, and balance will happen afterwards.
        _isWithinScrollChange = TRUE;
        IFC(_tpScrollViewer->ChangeViewWithOptionalAnimation(
            nullptr, spVerticalOffset.Get(), nullptr, TRUE /* disableAnimation */, &didSucceed));

        // If ChangeView doesn't succeed it implies the ScrollViewer is no longer in the visual tree.
        // We delay the setting of the scroll position until after it's placed in the visual tree again.
        if (!didSucceed)
        {
            _delayScrollPositionY = offset;
        }
        _isWithinScrollChange = FALSE;
    }
    else
    {
        // We call animate from the OnTapped event of children elements. This event
        // is generated when the gesture recognizer processes the PointerReleased event.
        // Unfortunately at this point in time the InputManager hasn't informed itself
        // the viewport is no longer active. As a result ScrollViewer will skip
        // the animation. To avoid this we schedule the ChangeView on the core dispatcher
        // for execution immediately after this tick completes.
        wrl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
        wrl::ComPtr<msy::IDispatcherQueue> spDispatcherQueue;
        boolean enqueued;
        wrl::ComPtr<LoopingSelector> spThis(this);
        wrl::WeakRef wrThis;

        IFC(spThis.AsWeak(&wrThis));

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            &spDispatcherQueueStatics));
        IFC(spDispatcherQueueStatics->GetForCurrentThread(&spDispatcherQueue));
        IFC(spDispatcherQueue->TryEnqueue(
            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wrThis, spVerticalOffset] () mutable {
                HRESULT hr = S_OK;
                BOOLEAN returnValue = FALSE;
                wrl::ComPtr<ILoopingSelector> spThis;
                IFC(wrThis.As(&spThis));

                if (spThis)
                {
                    IFC((static_cast<LoopingSelector*>(spThis.Get()))->_tpScrollViewer->ChangeViewWithOptionalAnimation(
                        nullptr, spVerticalOffset.Get(), nullptr, FALSE /* disableAnimation */, &returnValue));
                }
            Cleanup:
                RRETURN(hr);
            }).Get(),
            &enqueued));
        IFCEXPECT(enqueued);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::SetupSnapPoints(_In_ double offset, _In_ double size)
{
    HRESULT hr = S_OK;

    LoopingSelectorPanel* lsp = nullptr;
    lsp = static_cast<LoopingSelectorPanel*>(_tpPanel.Get());

    IFC(lsp->SetOffsetInPixels(static_cast<FLOAT>(offset)));
    IFC(lsp->SetSizeInPixels(static_cast<FLOAT>(size)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::SetPosition(_In_ xaml::IUIElement* pitem, _In_ double offset)
{
    HRESULT hr = S_OK;

    ASSERT(_spCanvasStatics);
    IFC(_spCanvasStatics->SetTop(pitem, offset));

    // Items are set offset with a large left offset
    // when recycled.
    IFC(_spCanvasStatics->SetLeft(pitem, 0.0));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::ExpandIfNecessary()
{
    if (_itemState != ItemState::Expanded)
    {
        IFC_RETURN(TransitionItemsState(ItemState::Expanded));
        _itemState = ItemState::Expanded;
    }

    return S_OK;
}

_Check_return_ HRESULT LoopingSelector::TransitionItemsState(_In_ ItemState state)
{
    HRESULT hr = S_OK;

    UINT32 childIdx = 0;
    std::vector<ILoopingSelectorItem*>::iterator iter;

    for (iter = _realizedItems.begin(); iter != _realizedItems.end(); iter++)
    {
        LoopingSelectorItem* lsi = static_cast<LoopingSelectorItem*>(*iter);

        if (state == ItemState::ManipulationInProgress)
        {
            IFC(lsi->SetState(LoopingSelectorItem::State::Expanded, TRUE));
        }
        else if (state == ItemState::Expanded)
        {
            if (_realizedTopIdx + (_realizedItems.size() - 1 - childIdx) == static_cast<UINT32>(_realizedMidpointIdx))
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Selected, TRUE));
            }
            else
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Expanded, TRUE));
            }
        }
        else // collapsed
        {
            if (_realizedTopIdx + (_realizedItems.size() - 1 - childIdx) == static_cast<UINT32>(_realizedMidpointIdx))
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Selected, TRUE));
            }
            else
            {
                IFC(lsi->SetState(LoopingSelectorItem::State::Normal, TRUE));
            }
        }

        childIdx++;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationGetSelectedItem(_Outptr_result_maybenull_ LoopingSelectorItem** ppItemNoRef)
{
    *ppItemNoRef = nullptr;

    for (auto iter = _realizedItems.begin(); iter != _realizedItems.end(); iter++)
    {
        LoopingSelectorItem* pLSINoRef = static_cast<LoopingSelectorItem*>(*iter);

        INT itemVisualIndex = 0;
        IFC_RETURN(pLSINoRef->GetVisualIndex(&itemVisualIndex));

        UINT32 itemIndex = 0;
        IFC_RETURN(VisualIndexToItemIndex(itemVisualIndex, &itemIndex));

        // We need to make sure that we check against the selected index,
        // not the midpoint index.  Normally, the item in the center of the view
        // is also the selected item, but in the case of UI automation, it
        // is not always guaranteed to be.
        INT32 selectedIndex = 0;
        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        if (itemIndex == static_cast<UINT32>(selectedIndex))
        {
            *ppItemNoRef = pLSINoRef;
            break;
        }
    }

    return S_OK;
}

HRESULT xaml_primitives::LoopingSelector::RetreiveItemFromAPRealizedItems(UINT32 moddeItemdIdx, ILoopingSelectorItem ** ppItem)
{
    *ppItem = nullptr;
    std::map<int, wrl::ComPtr<ILoopingSelectorItem>>::iterator iter;

    iter = _realizedItemsForAP.find(moddeItemdIdx);
    if (iter != _realizedItemsForAP.end())
    {
        *ppItem = iter->second.Detach();
        _realizedItemsForAP.erase(iter);
    }

    RRETURN(S_OK);
}

#pragma region Sound Helpers

_Check_return_ HRESULT LoopingSelector::RequestInteractionSound(xaml::ElementSoundKind soundKind)
{
    wrl::ComPtr<xaml::IDependencyObject> thisAsDO;

    IFC_RETURN(QueryInterface(__uuidof(xaml::IDependencyObject), &thisAsDO));
    IFC_RETURN(PlatformHelpers::RequestInteractionSoundForElement(soundKind, thisAsDO.Get()));

    return S_OK;
}

#pragma endregion Sound Helpers

#pragma region AutomationInternalInterface

_Check_return_ HRESULT LoopingSelector::AutomationScrollToVisualIdx(_In_ INT visualIdx, bool ignoreScrollingState)
{
    HRESULT hr = S_OK;

    BOOLEAN isFullySetup = FALSE;
    IFC(IsSetupForAutomation(&isFullySetup));

    if (isFullySetup && _itemState == ItemState::Expanded)
    {
        INT idxMovement = visualIdx - _realizedMidpointIdx;
        DOUBLE pixelsToMove = idxMovement * _scaledItemHeight;

        IFC(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE /* use animation */ ));

        IFC(Balance(TRUE /* isOnSnapPoint */ ));

        // If we aren't going to scroll at all, then we need to update the selected index,
        // since we won't get a ViewChanged event during which to do that.
        if (pixelsToMove == 0)
        {
            IFC(UpdateSelectedItem(ignoreScrollingState));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationGetIsScrollable(_Out_ BOOLEAN* pIsScrollable)
{
    // LoopingSelector doesn't currently have a disabled
    // state so as long as the itemCount is greater than
    // zero it is scrollable.
    *pIsScrollable = _itemCount > 0;
    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelector::AutomationGetScrollPercent(_Out_ DOUBLE* pScrollPercent)
{
    HRESULT hr = S_OK;

    INT32 selectedIndex;
    // We assume the scroll percent can be derived from the currently
    // selected item, since it's always in the middle.
    IFC(get_SelectedIndex(&selectedIndex));

    if (selectedIndex < 0)
    {
        selectedIndex = 0;
    }

    if (_itemCount > 0)
    {
        *pScrollPercent = static_cast<DOUBLE>(selectedIndex) / static_cast<DOUBLE>(_itemCount) * 100.0;
    }
    else
    {
        *pScrollPercent = 0.0;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationGetScrollViewSize(_Out_ DOUBLE* pScrollPercent)
{
    HRESULT hr = S_OK;

    BOOLEAN isSetup = FALSE;
    *pScrollPercent = 100.0;
    IFC(IsSetupForAutomation(&isSetup));
    if (isSetup)
    {
        DOUBLE viewportHeight = _unpaddedExtentBottom - _unpaddedExtentTop;
        if (viewportHeight > 0)
        {
            *pScrollPercent = viewportHeight / (_itemCount * _scaledItemHeight) * 100;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationSetScrollPercent(_In_ DOUBLE scrollPercent)
{
    HRESULT hr = S_OK;

    BOOLEAN isSetup = FALSE;

    if (scrollPercent < 0.0 || scrollPercent > 100.0)
    {
        IFC_NOTRACE(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

    IFC(IsSetupForAutomation(&isSetup));

    if (isSetup && _itemState == ItemState::Expanded)
    {
        INT itemIdxOffset = static_cast<INT>((_itemCount - 1) * scrollPercent / 100.0);
        INT currentItemIdx = PositiveMod(_realizedMidpointIdx, _itemCount);
        INT idxMovement = itemIdxOffset - currentItemIdx;
        DOUBLE pixelsToMove = idxMovement * _scaledItemHeight;
        IFC(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE /* use animation */ ));
        IFC(Balance(TRUE /* isOnSnapPoint */ ));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationTryGetSelectionUIAPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppPeer)
{
    HRESULT hr = S_OK;

    *ppPeer = nullptr;

    LoopingSelectorItem* pLSINoRef = nullptr;
    IFC(AutomationGetSelectedItem(&pLSINoRef));

    if (pLSINoRef)
    {
        wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerStatics> spAutomationPeerStatics;
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
        wrl::ComPtr<xaml::IUIElement> spChildAsUI;

        IFC(pLSINoRef->QueryInterface(
            __uuidof(xaml::IUIElement),
            &spChildAsUI));
        IFC(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
              &spAutomationPeerStatics));
        IFC(spAutomationPeerStatics->CreatePeerForElement(spChildAsUI.Get(), &spAutomationPeer));
        IFC(spAutomationPeer.CopyTo(ppPeer));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationScroll(_In_ xaml::Automation::ScrollAmount scrollAmount)
{
    HRESULT hr = S_OK;

    BOOLEAN isSetup = FALSE;
    IFC(IsSetupForAutomation(&isSetup));

    // We don't allow automation interaction when the ScrollViewer is undergoing
    // a manipulation.
    if (isSetup && _itemState == ItemState::Expanded)
    {
        DOUBLE pixelsToMove = 0.0;
        INT itemsToMove = 0;

        switch(scrollAmount)
        {
        case xaml::Automation::ScrollAmount_LargeDecrement:
            itemsToMove = -c_automationLargeIncrement;
            break;
        case xaml::Automation::ScrollAmount_LargeIncrement:
            itemsToMove = c_automationLargeIncrement;
            break;
        case xaml::Automation::ScrollAmount_SmallDecrement:
            itemsToMove = -c_automationSmallIncrement;
            break;
        case xaml::Automation::ScrollAmount_SmallIncrement:
            itemsToMove = c_automationSmallIncrement;
            break;
        default:
            break;
        }

        INT currentIndex = PositiveMod(_realizedMidpointIdx, _itemCount);

        if (currentIndex + itemsToMove > static_cast<INT>(_itemCount - 1))
        {
            itemsToMove = _itemCount - currentIndex - 1;
        }
        else if (currentIndex + itemsToMove < 0)
        {
            itemsToMove = -currentIndex;
        }

        pixelsToMove = itemsToMove * _scaledItemHeight;

        IFC(SetScrollPosition(_unpaddedExtentTop + pixelsToMove, FALSE /* use animation */ ));
        IFC(Balance(TRUE /* isOnSnapPoint */ ));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationFillCollectionWithRealizedItems(_In_ wfc::IVector<IInspectable*> *pVector)
{
    HRESULT hr = S_OK;

    // When the number of items is smaller than the number of items LoopingSelector calculates it
    // needs to realize to ensure gap-less scrolling the realizedItem list will contain duplicate items.
    // This will cause infinite loops when UIA clients attempt to find an element because of the logic UIAutomationCore
    // uses to traverse the UIA tree. Because the realizedItem list is in order we simply add items until we reach
    // a point where we've added either all available items or added the number of items in the data list.
    UINT32 counter = 0;
    for (auto iter = _realizedItems.begin(); iter != _realizedItems.end() && counter < _itemCount; iter++)
    {
        wrl::ComPtr<xaml_controls::IContentControl> spChildAsCC;
        wrl::ComPtr<IInspectable> spChildContent;

        counter++;

        IFC((*iter)->QueryInterface<xaml_controls::IContentControl>(
            &spChildAsCC));
        IFC(spChildAsCC->get_Content(&spChildContent));

        IFC(pVector->Append(spChildContent.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationTryScrollItemIntoView(_In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;

    UINT index = 0;
    BOOLEAN found = FALSE;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spVector;
    IFC(get_Items(&spVector));

    IFC(spVector->IndexOf(pItem, &index, &found));

    if (found)
    {
        _skipSelectionChangeUntilFinalViewChanged = true;

        // The _realizedMidpointIdx points to the currently selected item's visual index. Because the visual index
        // always starts at the first item we subtract it from itself modded with the item count to obtain the
        // nearest first item visual index.
        const INT desiredVisualIdx = _realizedMidpointIdx - PositiveMod(_realizedMidpointIdx, _itemCount) + index;
        IFC(AutomationScrollToVisualIdx(desiredVisualIdx));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationRaiseSelectionChanged()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IUIElement> spThisAsUI;
    wrl::ComPtr<xaml::Automation::IScrollPatternIdentifiersStatics> spScrollPatternStatics;
    wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationScrollProperty;

    wrl::ComPtr<xaml::IUIElement> spSelectedItemAsUI;
    LoopingSelectorItem* pLSINoRef = nullptr;

    wrl::ComPtr<IInspectable> spNewScrollValue;

    DOUBLE scrollPercent = 0.0;

    IFC(QueryInterface(
        __uuidof(xaml::IUIElement),
        &spThisAsUI));

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_ScrollPatternIdentifiers).Get(),
        &spScrollPatternStatics));
    IFC(spScrollPatternStatics->get_VerticalScrollPercentProperty(&spAutomationScrollProperty));

    IFC(AutomationGetScrollPercent(&scrollPercent));
    IFC(Private::ValueBoxer::CreateDouble(scrollPercent, &spNewScrollValue));

    IFC(AutomationGetSelectedItem(&pLSINoRef));
    if (pLSINoRef)
    {
        IFC(pLSINoRef->QueryInterface(
            __uuidof(xaml::IUIElement),
            &spSelectedItemAsUI));

        IFC(Private::AutomationHelper::RaiseEventIfListener(
            spSelectedItemAsUI.Get(),
            xaml::Automation::Peers::AutomationEvents_SelectionItemPatternOnElementSelected));
        IFC(Private::AutomationHelper::SetAutomationFocusIfListener(
            spSelectedItemAsUI.Get()));
    }

    IFC(Private::AutomationHelper::RaisePropertyChangedIfListener(
        spThisAsUI.Get(),
        spAutomationScrollProperty.Get(),
        _tpPreviousScrollPosition.Get(),
        spNewScrollValue.Get()));

    IFC(SetPtrValue(_tpPreviousScrollPosition, spNewScrollValue.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationRaiseExpandCollapse()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IUIElement> spThisAsUI;
    wrl::ComPtr<xaml::Automation::IExpandCollapsePatternIdentifiersStatics> spExpandCollapsePatternStatics;
    wrl::ComPtr<xaml::Automation::IAutomationProperty> spAutomationProperty;

    wrl::ComPtr<IInspectable> spOldValue;
    wrl::ComPtr<IInspectable> spNewValue;
    wrl::ComPtr<wf::IPropertyValue> spOldValueAsPV;
    wrl::ComPtr<wf::IPropertyValue> spNewValueAsPV;

    IFC(QueryInterface(
        __uuidof(xaml::IUIElement),
        &spThisAsUI));

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_ExpandCollapsePatternIdentifiers).Get(),
        &spExpandCollapsePatternStatics));

    IFC(spExpandCollapsePatternStatics->get_ExpandCollapseStateProperty(&spAutomationProperty));

    IFC(Private::ValueBoxer::CreateReference<xaml::Automation::ExpandCollapseState>
        (xaml::Automation::ExpandCollapseState_Collapsed, &spOldValueAsPV));
    IFC(Private::ValueBoxer::CreateReference<xaml::Automation::ExpandCollapseState>
        (xaml::Automation::ExpandCollapseState_Expanded, &spNewValueAsPV));

    IFC(spOldValueAsPV.As(&spOldValue));
    IFC(spNewValueAsPV.As(&spNewValue));

    IFC(Private::AutomationHelper::RaisePropertyChangedIfListener(
        spThisAsUI.Get(),
        spAutomationProperty.Get(),
        spOldValue.Get(),
        spNewValue.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationRaiseStructureChanged()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IUIElement> spThisAsUI;
    IFC(QueryInterface(
        __uuidof(xaml::IUIElement),
        &spThisAsUI));

    // The visible children has changed. Notify UIA of
    // a new structure.
    IFC(Private::AutomationHelper::RaiseEventIfListener(
        spThisAsUI.Get(),
        xaml::Automation::Peers::AutomationEvents_StructureChanged));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelector::AutomationGetContainerUIAPeerForItem(
    _In_ IInspectable* pItem,
    _Outptr_result_maybenull_ xaml_automation_peers::ILoopingSelectorItemAutomationPeer** ppPeer)
{
    *ppPeer = nullptr;
    wrl::ComPtr<ILoopingSelectorItem> spChild;

    for (auto iter = _realizedItems.begin(); iter != _realizedItems.end(); iter++)
    {
        wrl::ComPtr<ILoopingSelectorItem> spTentativeChild(*iter);
        wrl::ComPtr<xaml_controls::IContentControl> spTentativeChildAsCC;
        wrl::ComPtr<IInspectable> spItem;
        IFC_RETURN(spTentativeChild.As(&spTentativeChildAsCC));

        IFC_RETURN(spTentativeChildAsCC->get_Content(&spItem));

        if (spItem.Get() == pItem)
        {
            spChild = spTentativeChild;
            break;
        }
    }

    if (!spChild)
    {
        for (auto iter = _realizedItemsForAP.begin(); iter != _realizedItemsForAP.end(); iter++)
        {
            wrl::ComPtr<ILoopingSelectorItem> spTentativeChild(iter->second);
            wrl::ComPtr<xaml_controls::IContentControl> spTentativeChildAsCC;
            wrl::ComPtr<IInspectable> spItem;
            IFC_RETURN(spTentativeChild.As(&spTentativeChildAsCC));

            IFC_RETURN(spTentativeChildAsCC->get_Content(&spItem));

            if (spItem.Get() == pItem)
            {
                spChild = spTentativeChild;
                break;
            }
        }
    }

    if (spChild)
    {
        wrl::ComPtr<xaml::IUIElement> spChildAsUIElt;
        wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spChildAP;
        wrl::ComPtr<xaml_automation_peers::ILoopingSelectorItemAutomationPeer> spChildAPAsLSIAP;
        IFC_RETURN(spChild.As(&spChildAsUIElt));
        IFC_RETURN(Private::AutomationHelper::CreatePeerForElement(spChildAsUIElt.Get(),
            &spChildAP));
        IFC_RETURN(spChildAP.As(&spChildAPAsLSIAP));

        *ppPeer = spChildAPAsLSIAP.Detach();
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelector::AutomationClearPeerMap()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorAutomationPeer> spLSAP;
    IFC(_wrAP.CopyTo<xaml_automation_peers::ILoopingSelectorAutomationPeer>(&spLSAP));

    if (spLSAP)
    {
        IFC(static_cast<xaml_automation_peers::LoopingSelectorAutomationPeer*>(spLSAP.Get())->ClearPeerMap());
    }

Cleanup:
    RRETURN(hr);
}

HRESULT xaml_primitives::LoopingSelector::AutomationRealizeItemForAP(UINT32 itemIdxToRealize)
{
    wrl::ComPtr<ILoopingSelectorItem> spItem;
    IFC_RETURN(RealizeItem(itemIdxToRealize, &spItem));
    _realizedItemsForAP[itemIdxToRealize] = spItem;
    RRETURN(S_OK);
}

#pragma endregion

} } } } } XAML_ABI_NAMESPACE_END