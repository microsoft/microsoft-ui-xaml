// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// Herein are contained a set of classes that assist with overriding methods from XAML controls.
// These are hand-authored on an as-needed basis.
//

// NOTE: We use a weird pattern here where the second template parameter is the interface we are
// implementing. The problem is we want __uuidof(OverridesHelper) to work because we want the
// OverridesHelper type to be listed in the RuntimeClass interface list. Naively trying to use
// __declspec(uuid()) on the type itself doesn't work because it doesn't get carried over to the
// template specializations. However, there is a feature (bug?) in the MSVC compiler where 
// __uuidof(...) will search template parameters for uuid attributes. So we take advantage of that
// and list the ABI interface as one of our template parameters.


// ============================================================================
// IFrameworkElementOverridesHelper
// ============================================================================

template <typename BASE, 
    typename OverridesInterface = abi::IFrameworkElementOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
struct 
__declspec(novtable)
IFrameworkElementOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(MeasureOverride)(abi::Size availableSize, abi::Size* desiredSize)
    {
        auto spOverrides = GetComposableInner().as<abi::IFrameworkElementOverrides>();
        return spOverrides->MeasureOverride(availableSize, desiredSize);
    }
    IFACEMETHOD(ArrangeOverride)(abi::Size finalSize, abi::Size* returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IFrameworkElementOverrides>();
        return spOverrides->ArrangeOverride(finalSize, returnValue);
    }
    IFACEMETHOD(OnApplyTemplate)()
    {
        auto spOverrides = GetComposableInner().as<abi::IFrameworkElementOverrides>();
        return spOverrides->OnApplyTemplate();
    }
};

// ============================================================================
// IUIElementOverridesHelper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IUIElementOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IUIElementOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(OnCreateAutomationPeer)(abi::IAutomationPeer** returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IUIElementOverrides>();
        return spOverrides->OnCreateAutomationPeer(returnValue);
    }

    IFACEMETHOD(OnDisconnectVisualChildren)()
    {
        auto spOverrides = GetComposableInner().as<abi::IUIElementOverrides>();
        return spOverrides->OnDisconnectVisualChildren();
    }

    IFACEMETHOD(FindSubElementsForTouchTargeting)(abi::Point point, abi::Rect boundingRect, abi::IIterable<abi::IIterable<::abi::Point>*>** ppReturnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IUIElementOverrides>();
        return spOverrides->FindSubElementsForTouchTargeting(point, boundingRect, ppReturnValue);
    }
};

// ============================================================================
// IUIElementOverrides7Helper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IUIElementOverrides7> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IUIElementOverrides7Helper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(GetChildrenInTabFocusOrder)(
        abi::IIterable<abi::DependencyObject*>** returnValue) override
    {
        auto overrides = GetComposableInner().as<abi::IUIElementOverrides7>();
        return overrides->GetChildrenInTabFocusOrder(returnValue);
    }

    IFACEMETHOD(OnProcessKeyboardAccelerators)(
        abi::IProcessKeyboardAcceleratorEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IUIElementOverrides7>();
        return overrides->OnProcessKeyboardAccelerators(args);
    }
};

// ============================================================================
// IUIElementOverrides8Helper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IUIElementOverrides8> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IUIElementOverrides8Helper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(OnKeyboardAcceleratorInvoked)(abi::IKeyboardAcceleratorInvokedEventArgs* e) override
    {
        auto overrides = GetComposableInner().as<abi::IUIElementOverrides8>();
        return overrides->OnKeyboardAcceleratorInvoked(e);
    }

    IFACEMETHOD(OnBringIntoViewRequested)(abi::IBringIntoViewRequestedEventArgs* e) override
    {
        auto overrides = GetComposableInner().as<abi::IUIElementOverrides8>();
        return overrides->OnBringIntoViewRequested(e);
    }
};

// ============================================================================
// IControlOverridesHelper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IControlOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IControlOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(OnKeyDown)(abi::IKeyRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnKeyDown(e);
    }

    IFACEMETHOD(OnPointerEntered)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerEntered(e);
    }

    IFACEMETHOD(OnPointerPressed)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerPressed(e);
    }

    IFACEMETHOD(OnPointerMoved)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerMoved(e);
    }

    IFACEMETHOD(OnPointerReleased)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerReleased(e);
    }

    IFACEMETHOD(OnPointerExited)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerExited(e);
    }

    IFACEMETHOD(OnPointerCaptureLost)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerCaptureLost(e);
    }

    IFACEMETHOD(OnPointerCanceled)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerCanceled(e);
    }

    IFACEMETHOD(OnPointerWheelChanged)(abi::IPointerRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnPointerWheelChanged(e);
    }

    IFACEMETHOD(OnTapped)(abi::ITappedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnTapped(e);
    }

    IFACEMETHOD(OnDoubleTapped)(abi::IDoubleTappedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnDoubleTapped(e);
    }

    IFACEMETHOD(OnHolding)(abi::IHoldingRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnHolding(e);
    }

    IFACEMETHOD(OnRightTapped)(abi::IRightTappedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnRightTapped(e);
    }

    IFACEMETHOD(OnManipulationStarting)(abi::IManipulationStartingRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnManipulationStarting(e);
    }

    IFACEMETHOD(OnManipulationInertiaStarting)(abi::IManipulationInertiaStartingRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnManipulationInertiaStarting(e);
    }

    IFACEMETHOD(OnManipulationStarted)(abi::IManipulationStartedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnManipulationStarted(e);
    }

    IFACEMETHOD(OnManipulationDelta)(abi::IManipulationDeltaRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnManipulationDelta(e);
    }

    IFACEMETHOD(OnManipulationCompleted)(abi::IManipulationCompletedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnManipulationCompleted(e);
    }

    IFACEMETHOD(OnKeyUp)(abi::IKeyRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnKeyUp(e);
    }

    IFACEMETHOD(OnGotFocus)(abi::IRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnGotFocus(e);
    }

    IFACEMETHOD(OnLostFocus)(abi::IRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnLostFocus(e);
    }

    IFACEMETHOD(OnDragEnter)(abi::IDragEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnDragEnter(e);
    }

    IFACEMETHOD(OnDragLeave)(abi::IDragEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnDragLeave(e);
    }

    IFACEMETHOD(OnDragOver)(abi::IDragEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnDragOver(e);
    }

    IFACEMETHOD(OnDrop)(abi::IDragEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides>();
        return spOverrides->OnDrop(e);
    }

};

// ============================================================================
// IControlOverrides6Helper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IControlOverrides6> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IControlOverrides6Helper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(OnPreviewKeyDown)(abi::IKeyRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides6>();
        return spOverrides->OnPreviewKeyDown(e);
    }

    IFACEMETHOD(OnPreviewKeyUp)(abi::IKeyRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides6>();
        return spOverrides->OnPreviewKeyUp(e);
    }

    IFACEMETHOD(OnCharacterReceived)(abi::ICharacterReceivedRoutedEventArgs *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IControlOverrides6>();
        return spOverrides->OnCharacterReceived(e);
    }
};

// ============================================================================
// IAutomationPeerOverridesHelper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IAutomationPeerOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IAutomationPeerOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(GetPatternCore)(abi::PatternInterface patternInterface, IInspectable **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetPatternCore(patternInterface, returnValue);
    }

    IFACEMETHOD(GetAcceleratorKeyCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetAcceleratorKeyCore(returnValue);
    }

    IFACEMETHOD(GetAccessKeyCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetAccessKeyCore(returnValue);
    }

    IFACEMETHOD(GetAutomationControlTypeCore)(abi::AutomationControlType *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetAutomationControlTypeCore(returnValue);
    }

    IFACEMETHOD(GetAutomationIdCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetAutomationIdCore(returnValue);
    }

    IFACEMETHOD(GetBoundingRectangleCore)(abi::Rect *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetBoundingRectangleCore(returnValue);
    }

    IFACEMETHOD(GetChildrenCore)(__FIVector_1_Windows__CUI__CXaml__CAutomation__CPeers__CAutomationPeer **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetChildrenCore(returnValue);
    }

    IFACEMETHOD(GetClassNameCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetClassNameCore(returnValue);
    }

    IFACEMETHOD(GetClickablePointCore)(abi::Point *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetClickablePointCore(returnValue);
    }

    IFACEMETHOD(GetHelpTextCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetHelpTextCore(returnValue);
    }

    IFACEMETHOD(GetItemStatusCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetItemStatusCore(returnValue);
    }

    IFACEMETHOD(GetItemTypeCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetItemTypeCore(returnValue);
    }

    IFACEMETHOD(GetLabeledByCore)(abi::IAutomationPeer **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetLabeledByCore(returnValue);
    }

    IFACEMETHOD(GetLocalizedControlTypeCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetLocalizedControlTypeCore(returnValue);
    }

    IFACEMETHOD(GetNameCore)(HSTRING *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetNameCore(returnValue);
    }

    IFACEMETHOD(GetOrientationCore)(abi::AutomationOrientation *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetOrientationCore(returnValue);
    }

    IFACEMETHOD(HasKeyboardFocusCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->HasKeyboardFocusCore(returnValue);
    }

    IFACEMETHOD(IsContentElementCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsContentElementCore(returnValue);
    }

    IFACEMETHOD(IsControlElementCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsControlElementCore(returnValue);
    }

    IFACEMETHOD(IsEnabledCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsEnabledCore(returnValue);
    }

    IFACEMETHOD(IsKeyboardFocusableCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsKeyboardFocusableCore(returnValue);
    }

    IFACEMETHOD(IsOffscreenCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsOffscreenCore(returnValue);
    }

    IFACEMETHOD(IsPasswordCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsPasswordCore(returnValue);
    }

    IFACEMETHOD(IsRequiredForFormCore)(boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->IsRequiredForFormCore(returnValue);
    }

    IFACEMETHOD(SetFocusCore)()
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->SetFocusCore();
    }

    IFACEMETHOD(GetPeerFromPointCore)(abi::Point point, abi::IAutomationPeer **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetPeerFromPointCore(point, returnValue);
    }

    IFACEMETHOD(GetLiveSettingCore)(abi::AutomationLiveSetting *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetLiveSettingCore(returnValue);
    }
};

// ============================================================================
// IAutomationPeerOverrides3Helper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IAutomationPeerOverrides3> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IAutomationPeerOverrides3Helper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(GetPatternCore)(abi::PatternInterface patternInterface, IInspectable **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides>();
        return spOverrides->GetPatternCore(patternInterface, returnValue);
    }

    IFACEMETHOD(NavigateCore)(abi::AutomationNavigationDirection direction, IInspectable **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->NavigateCore(direction, returnValue);
    }

    IFACEMETHOD(GetElementFromPointCore)(abi::Point pointInWindowCoordinates, IInspectable **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetElementFromPointCore(pointInWindowCoordinates, returnValue);
    }

    IFACEMETHOD(GetFocusedElementCore)(IInspectable **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetFocusedElementCore(returnValue);
    }

    IFACEMETHOD(GetAnnotationsCore)(__FIVector_1_Windows__CUI__CXaml__CAutomation__CPeers__CAutomationPeerAnnotation **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetAnnotationsCore(returnValue);
    }

    IFACEMETHOD(GetPositionInSetCore)(INT32 *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetPositionInSetCore(returnValue);
    }

    IFACEMETHOD(GetSizeOfSetCore)(INT32 *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetSizeOfSetCore(returnValue);
    }

    IFACEMETHOD(GetLevelCore)(INT32 *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IAutomationPeerOverrides3>();
        return spOverrides->GetLevelCore(returnValue);
    }

};

// ============================================================================
// IItemsControlOverridesHelper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IItemsControlOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IItemsControlOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(IsItemItsOwnContainerOverride)(IInspectable *item, boolean *returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->IsItemItsOwnContainerOverride(item, returnValue);
    }

    IFACEMETHOD(GetContainerForItemOverride)(abi::IDependencyObject **returnValue)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->GetContainerForItemOverride(returnValue);
    }

    IFACEMETHOD(ClearContainerForItemOverride)(abi::IDependencyObject *element, IInspectable *item)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->ClearContainerForItemOverride(element, item);
    }

    IFACEMETHOD(PrepareContainerForItemOverride)(abi::IDependencyObject *element, IInspectable *item)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->PrepareContainerForItemOverride(element, item);
    }

    IFACEMETHOD(OnItemsChanged)(IInspectable *e)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnItemsChanged(e);
    }

    IFACEMETHOD(OnItemContainerStyleChanged)(abi::IStyle *oldItemContainerStyle, abi::IStyle *newItemContainerStyle)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnItemContainerStyleChanged(oldItemContainerStyle, newItemContainerStyle);
    }

    IFACEMETHOD(OnItemContainerStyleSelectorChanged)(abi::IStyleSelector *oldItemContainerStyleSelector, abi::IStyleSelector *newItemContainerStyleSelector)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnItemContainerStyleSelectorChanged(oldItemContainerStyleSelector, newItemContainerStyleSelector);
    }

    IFACEMETHOD(OnItemTemplateChanged)(abi::IDataTemplate *oldItemTemplate, abi::IDataTemplate *newItemTemplate)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnItemTemplateChanged(oldItemTemplate, newItemTemplate);
    }

    IFACEMETHOD(OnItemTemplateSelectorChanged)(abi::IDataTemplateSelector *oldItemTemplateSelector, abi::IDataTemplateSelector *newItemTemplateSelector)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnItemTemplateSelectorChanged(oldItemTemplateSelector, newItemTemplateSelector);
    }

    IFACEMETHOD(OnGroupStyleSelectorChanged)(abi::IGroupStyleSelector *oldGroupStyleSelector, abi::IGroupStyleSelector *newGroupStyleSelector)
    {
        auto spOverrides = GetComposableInner().as<abi::IItemsControlOverrides>();
        return spOverrides->OnGroupStyleSelectorChanged(oldGroupStyleSelector, newGroupStyleSelector);
    }
};

// ============================================================================
// IContentControlOverridesHelper
// ============================================================================

template <typename BASE,
    typename OverridesInterface = abi::IContentControlOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct
    __declspec(novtable)
    IContentControlOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(OnContentChanged)(IInspectable* oldContent, IInspectable* newContent)
    {
        auto spOverrides = GetComposableInner().as<abi::IContentControlOverrides>();
        return spOverrides->OnContentChanged(oldContent, newContent);
    }
    IFACEMETHOD(OnContentTemplateChanged)(abi::IDataTemplate* oldContentTemplate, abi::IDataTemplate* newContentTemplate)
    {
        auto spOverrides = GetComposableInner().as<abi::IContentControlOverrides>();
        return spOverrides->OnContentTemplateChanged(oldContentTemplate, newContentTemplate);
    }
    IFACEMETHOD(OnContentTemplateSelectorChanged)(abi::IDataTemplateSelector* oldContentTemplateSelector, abi::IDataTemplateSelector* newContentTemplateSelector)
    {
        auto spOverrides = GetComposableInner().as<abi::IContentControlOverrides>();
        return spOverrides->OnContentTemplateSelectorChanged(oldContentTemplateSelector, newContentTemplateSelector);
    }
};

// ============================================================================
// IInteractionBaseOverridesHelper
// ============================================================================
template <typename BASE, typename OverridesInterface = abi::IInteractionBaseOverrides> // If you're adding a new overrides helper, add references to CppWinRTIncludes.h
    struct __declspec(novtable) IInteractionBaseOverridesHelper : public OverridesInterface
{
    virtual winrt::com_ptr<IInspectable> GetComposableInner() = 0;

    IFACEMETHOD(GetSupportedEventsCore)(abi::IVectorView<abi::RoutedEvent*>** result)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->GetSupportedEventsCore(result);
    }

    IFACEMETHOD(OnKeyDown)(abi::IUIElement* sender, abi::IKeyRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnKeyDown(sender, args);
    }

    IFACEMETHOD(OnKeyUp)(abi::IUIElement* sender, abi::IKeyRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnKeyUp(sender, args);
    }

    IFACEMETHOD(OnPointerEntered)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerEntered(sender, args);
    }

    IFACEMETHOD(OnPointerExited)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerExited(sender, args);
    }

    IFACEMETHOD(OnPointerMoved)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerMoved(sender, args);
    }

    IFACEMETHOD(OnPointerPressed)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerPressed(sender, args);
    }

    IFACEMETHOD(OnPointerReleased)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerReleased(sender, args);
    }

    IFACEMETHOD(OnPointerCaptureLost)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerCaptureLost(sender, args);
    }

    IFACEMETHOD(OnPointerCanceled)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerCanceled(sender, args);
    }

    IFACEMETHOD(OnPointerWheelChanged)(abi::IUIElement* sender, abi::IPointerRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnPointerWheelChanged(sender, args);
    }

    IFACEMETHOD(OnTapped)(abi::IUIElement* sender, abi::ITappedRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnTapped(sender, args);
    }

    IFACEMETHOD(OnDoubleTapped)(abi::IUIElement* sender, abi::IDoubleTappedRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnDoubleTapped(sender, args);
    }

    IFACEMETHOD(OnHolding)(abi::IUIElement* sender, abi::IHoldingRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnHolding(sender, args);
    }

    IFACEMETHOD(OnRightTapped)(abi::IUIElement* sender, abi::IRightTappedRoutedEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnRightTapped(sender, args);
    }

    IFACEMETHOD(OnDragEnter)(abi::IUIElement* sender, abi::IDragEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnDragEnter(sender, args);
    }

    IFACEMETHOD(OnDragLeave)(abi::IUIElement* sender, abi::IDragEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnDragLeave(sender, args);
    }

    IFACEMETHOD(OnDragOver)(abi::IUIElement* sender, abi::IDragEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnDragOver(sender, args);
    }

    IFACEMETHOD(OnDrop)(abi::IUIElement* sender, abi::IDragEventArgs* args)
    {
        auto overrides = GetComposableInner().as<abi::IInteractionBaseOverrides>();
        return overrides->OnDrop(sender, args);
    }
};
