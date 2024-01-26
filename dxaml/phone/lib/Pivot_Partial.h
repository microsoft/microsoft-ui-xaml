// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PivotCommon.h>
#include <PivotStateMachine.h>
#include "PivotHeaderManager.h"
#include "PivotHeaderManagerCallbacks.h"

#include <windows.graphics.display.h>
#include <microsoft.ui.composition.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

struct IPivotAnimatorCallbacks
{
    _Check_return_ virtual HRESULT OnAnimationComplete() = 0;
};

class PivotAnimator
{
public:
    PivotAnimator(IPivotAnimatorCallbacks* pCallbacks, Private::ReferenceTrackerHelper<Pivot> referenceTrackerHelper);
    ~PivotAnimator();

    _Check_return_ HRESULT SetTargets(
        _In_opt_ xaml_media::ITranslateTransform* pTranslateTransform,
        _In_opt_ xaml_controls::IItemsPresenter* pItemsPresenter,
        _In_opt_ xaml_media::ICompositeTransform* pHeaderTransform);

    _Check_return_ HRESULT AnimateOut(_In_ DOUBLE startOffset, _In_ DOUBLE targetOffset, _In_ bool fromLeft);
    _Check_return_ HRESULT AnimateIn(_In_ DOUBLE targetOffset, _In_ bool toLeft);

    _Check_return_ HRESULT Stop();
    _Check_return_ HRESULT Complete();

private:
    _Check_return_ HRESULT OnCompleted(_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2);

    template <typename T>
    _Check_return_ HRESULT SetPtrValue(_In_ Private::TrackerPtr<T>& ptr, _In_ T* value)
    {
        RRETURN(m_referenceTrackerHelper.SetPtrValue(ptr, value));
    }

    _Check_return_ HRESULT CreateAnimation(
        _In_ xaml::IDependencyObject* target,
        _In_ HSTRING targetProperty,
        _In_ unsigned int duration,
        _In_ bool useSpline,
        _Out_ xaml_animation::IDoubleAnimationUsingKeyFrames **ppAnimation,
        _Out_ xaml_animation::IDoubleKeyFrame **ppAnimationKeyFrameStart,
        _Out_ xaml_animation::IDoubleKeyFrame **ppAnimationKeyFrameFinish);

    IPivotAnimatorCallbacks* m_pCallbackPtr;
    Private::ReferenceTrackerHelper<Pivot> m_referenceTrackerHelper;

    Private::TrackerPtr<xaml_animation::IStoryboard> m_tpFlyOutStoryboardRunning;
    Private::TrackerPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> m_tpFlyOutTranslationAnimation;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyOutTranslationAnimationKeyFrameStart;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyOutTranslationAnimationKeyFrameFinish;

    Private::TrackerPtr<xaml_animation::IStoryboard> m_tpFlyOutHeaderStoryboardRunning;
    Private::TrackerPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> m_tpFlyOutHeaderTranslationAnimation;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyOutHeaderTranslationAnimationKeyFrameStart;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyOutHeaderTranslationAnimationKeyFrameFinish;

    Private::TrackerPtr<xaml_animation::IStoryboard> m_tpFlyInStoryboardRunning;
    Private::TrackerPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> m_tpFlyInTranslationAnimation;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyInTranslationAnimationKeyFrameStart;
    Private::TrackerPtr<xaml_animation::IDoubleKeyFrame> m_tpFlyInTranslationAnimationKeyFrameFinish;

    static const DOUBLE c_easingExponent;

    EventRegistrationToken m_flyOutCompletedToken;
    EventRegistrationToken m_flyInCompletedToken;

    static const UINT32 c_flyOutTranslationAnimationDuration;
    static const DOUBLE c_flyOutTranslationAnimationDistance;
    static const UINT32 c_flyOutOpacityAnimationDuration;

    static const UINT32 c_flyInTranslationAnimationDuration;
    static const DOUBLE c_flyInTranslationAnimationDistance;
    static const UINT32 c_flyInOpacityAnimationDuration;

};

class PivotSlideInThemeAnimation :
    public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::IThemeAnimationBaseOverrides,
        wrl::ComposableBase<xaml_animation::IThemeAnimationBaseFactory>
    >
{
    InspectableClass(L"Microsoft.UI.Xaml.Media.Timeline", TrustLevel::BaseTrust);
    friend class Pivot;

public:
    PivotSlideInThemeAnimation()
        : m_offsetX(0.0)
    { }

    _Check_return_ HRESULT RuntimeClassInitialize(_In_ DOUBLE offsetX);

protected:
    // IThemeAnimationBaseOverrides
    _Check_return_ IFACEMETHOD(CreateTimelinesInternal)(
        _In_ BOOLEAN onlyGenerateSteadyState,
        _In_ wfc::IVector<xaml_animation::Timeline*> *pTimelineCollection) override;

private:
    // The 'from' value on the x-axis. The 'to' value is zero.
    DOUBLE m_offsetX;

    _Check_return_ static HRESULT EnsureStaticsAndFactories();

    static wrl::ComPtr<xaml_animation::IThemeAnimationBaseFactory> s_spThemeAnimationBaseFactory;
};

// Holds information that we set on a FE that will slide-in once
// its parent PivotItem is selected.
class PivotSlideInElementInformation :
    public ::Microsoft::WRL::RuntimeClass<IInspectable>
{
    InspectableClass(L"IInspectable", TrustLevel::BaseTrust);
    friend class PivotSlideInManager;

public:
    PivotSlideInElementInformation()
        : m_loadedToken()
        , m_unloadedToken()
    {}

private:
    EventRegistrationToken m_loadedToken;
    EventRegistrationToken m_unloadedToken;

    wrl::WeakRef m_wrParentPivotItem;
};

class PivotSlideInManager
{
public:
    // Returns whether a value is valid or not for slide-in.
    // For some invalid values, this will result in an error being returned
    // (and, eventually, an exception getting raised).
    // Valid values are GroupOne, GroupTwo and GroupThree.
    // 'Invalid' values are DefaultGroup.
    // Invalid values that result in an error are everything else.
    _Check_return_ static HRESULT IsSlideInAnimationGroupValueValid(
        _In_ const wrl::ComPtr<IInspectable>& spValue,
        _Out_ BOOLEAN* pIsValid);
    _Check_return_ static HRESULT RegisterSlideInElement(_In_ const wrl::ComPtr<IFrameworkElement>& spElement);
    _Check_return_ static HRESULT UnregisterSlideInElement(_In_ const wrl::ComPtr<IFrameworkElement>& spElement);
    _Check_return_ static HRESULT PrepareSlideInElement(_In_ const wrl::ComPtr<xaml::IFrameworkElement>& spElement);
    _Check_return_ static HRESULT UnprepareSlideInElement(_In_ const wrl::ComPtr<xaml::IFrameworkElement>& spElement);
    _Check_return_ static HRESULT ApplySlideInAnimation(
        _In_ const wrl::ComPtr<IPivotItem>& spPivotItem,
        _In_ PivotAnimationDirection animationDirection);

private:
    _Check_return_ static HRESULT OnSlideInElementLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
    _Check_return_ static HRESULT OnSlideInElementUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
};

class PivotHeaderManager;

class Pivot :
    public PivotGenerated,
    public IPivotStateMachineCallbacks,
    public IPivotAnimatorCallbacks,
    public IPivotHeaderManagerCallbacks
{

public:
    Pivot();

    // Template part names are consumed by PivotPanel
    // at the moment and are made public for that reason.
    static const WCHAR c_PivotItemsPresenterName[];
    static const WCHAR c_TitleControlName[];
    static const WCHAR c_HeadersControlName[];
    static const WCHAR c_StaticHeadersControlName[];
    static const WCHAR c_ScrollViewerName[];
    static const WCHAR c_PanelName[];
    static const WCHAR c_PivotItemsPresenterTranslateTransformName[];
    static const WCHAR c_PivotItemsPresenterCompositeTransformName[];
    static const WCHAR c_HeaderTranslateTransformName[];
    static const WCHAR c_StaticHeaderTranslateTransformName[];
    static const WCHAR c_HeaderOffsetTranslateTransformName[];
    static const WCHAR c_NextButtonName[];
    static const WCHAR c_PreviousButtonName[];
    static const WCHAR c_LayoutElementName[];
    static const WCHAR c_LayoutElementTranslateTransformName[];
    static const WCHAR c_HeaderClipperName[];
    static const WCHAR c_HeaderClipperGeometryName[];
    static const WCHAR c_LeftHeaderPresenterName[];
    static const WCHAR c_RightHeaderPresenterName[];
    static const WCHAR c_FocusFollowerName[];

    static const WCHAR c_VisualStateLandscape[];
    static const WCHAR c_VisualStatePortrait[];

    _Check_return_ static HRESULT OnAttachedPropertyChanged(
        _In_ xaml::IDependencyObject* pSender,
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

    _Check_return_ static HRESULT GetDefaultIsHeaderItemsCarouselEnabled(
        _Outptr_ IInspectable** value);

    _Check_return_ static HRESULT GetDefaultSlideInAnimationGroup(
        _Outptr_ IInspectable** ppValue);

    _Check_return_ static HRESULT GetDefaultHeaderFocusVisualPlacement(
        _Outptr_ IInspectable** ppValue);

    // When we're measured with infinite size there's a couple
    // places where we substitute the available width with the
    // screen width.
    static _Check_return_ HRESULT GetScreenWidth(_In_ Pivot* const pivot, _Out_ DOUBLE* pWidth);
    static _Check_return_ HRESULT GetZoomScale(_Out_ DOUBLE* zoomScale);

    _Check_return_ HRESULT OnPointerEnteredHeader(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* e);
    _Check_return_ HRESULT OnPointerExitedHeader(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* e);
    _Check_return_ HRESULT OnPreviousButtonClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
    _Check_return_ HRESULT OnNextButtonClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

    _Check_return_ HRESULT InvalidateHeaderSecondaryContentRelationships();

    _Check_return_ HRESULT GetItemHeaderBoundingRectangle(_In_ INT index, _Out_ wf::Rect *boundingRectangle);
    _Check_return_ HRESULT GetItemHeaderClickablePoint(_In_ INT index, _Out_ wf::Point *clickablePoint);
    _Check_return_ HRESULT IsItemHeaderOffscreen(_In_ INT index, _Out_ bool *isOffscreen);

    _Check_return_ HRESULT HeaderPanelIsKeyboardFocusable(_Out_ bool *isKeyboardFocusable);
    _Check_return_ HRESULT HeaderPanelHasKeyboardFocus(_Out_ bool *hasKeyboardFocus);

    _Check_return_ HRESULT SetFocusToItem(_In_ INT index);

    unsigned GetPivotPanelMultiplierImpl() const;

    _Check_return_ HRESULT GetTitleControl(_Outptr_result_maybenull_ xaml_controls::IContentControl** titleControl);
    _Check_return_ HRESULT GetLeftHeaderPresenter(_Outptr_result_maybenull_ xaml::IFrameworkElement** leftHeaderPresenter);
    _Check_return_ HRESULT GetRightHeaderPresenter(_Outptr_result_maybenull_ xaml::IFrameworkElement** rightHeaderPresenter);

protected:
     _Check_return_ HRESULT InitializeImpl(_In_opt_ IInspectable* pOuter) override;

    // IFrameworkElementOverrides methods
    _Check_return_ HRESULT ArrangeOverrideImpl(
        _In_ wf::Size finalSize,
        _Out_ wf::Size* pReturnValue) override;
    _Check_return_ HRESULT MeasureOverrideImpl(
        _In_ wf::Size availableSize,
        _Out_ wf::Size* pDesiredSize) override;
    _Check_return_ HRESULT OnApplyTemplateImpl() override;

     // IItemsControlOverrides methods
    _Check_return_ HRESULT PrepareContainerForItemOverrideImpl(
        _In_ xaml::IDependencyObject* element,
        _In_ IInspectable *item) override;
    _Check_return_ HRESULT OnItemsChangedImpl(
        _In_ IInspectable* e) override;
    _Check_return_ HRESULT IsItemItsOwnContainerOverrideImpl(
        _In_opt_ IInspectable* item,
        _Out_ BOOLEAN* returnValue) override;
    _Check_return_ HRESULT GetContainerForItemOverrideImpl(
        _Outptr_ xaml::IDependencyObject** returnValue) override;

    // IUIElementOverrides methods
    _Check_return_ HRESULT OnCreateAutomationPeerImpl(
        _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

    // IControlOverrides
    _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* e) override;
    _Check_return_ HRESULT OnKeyUpImpl(_In_ xaml_input::IKeyRoutedEventArgs* e) override;

#pragma region Automation
public:
    _Check_return_ HRESULT AutomationGetIsScrollable(_Out_ BOOLEAN* pIsScrollable);
    _Check_return_ HRESULT AutomationGetScrollPercent(_Out_ DOUBLE* pScrollPercent);
    _Check_return_ HRESULT AutomationGetViewSize(_Out_ DOUBLE* pViewSize);
    _Check_return_ HRESULT AutomationScroll(_In_ xaml_automation::ScrollAmount scrollAmount);
    _Check_return_ HRESULT AutomationSetScrollPercent(_In_ DOUBLE scrollPercent);

private:
    static BOOLEAN AutomationCalculateIsScrollable(_In_ UINT itemsCount, _In_ BOOLEAN isLocked);
    static DOUBLE AutomationCalculateViewSize(_In_ UINT itemsCount, _In_ BOOLEAN isLocked);
    static DOUBLE AutomationCalculateScrollPercent(_In_ UINT itemsCount, _In_ BOOLEAN isLocked, _In_ INT selectedIndex);

    _Check_return_ HRESULT AutomationOnSelectionChanged(_In_ IInspectable* pOldItem,_In_ IInspectable* pNewItem);
    _Check_return_ HRESULT AutomationRaiseSelectionChangedEvent();
    _Check_return_ HRESULT AutomationRaisePropertyChangedEvents();

    _Check_return_ HRESULT AutomationFocusSelectedItem();
    _Check_return_ HRESULT AutomationFocusItem(_In_ INT index);
#pragma endregion

private:
#pragma region PivotPanel Helpers
    _Check_return_ HRESULT UpdateVisibleContent(_In_ INT index, _In_ PivotAnimationDirection animationHint);
    _Check_return_ HRESULT UpdateItemVisibility(_In_ xaml::IUIElement* pContainer, _In_ BOOLEAN toVisible, _Out_ BOOLEAN* pIsFirstExpand);
    _Check_return_ HRESULT UpdateFocusFollowerImpl(_In_ INT idx);
    _Check_return_ HRESULT HasItemsHost(_Out_ BOOLEAN* pValue);
    _Check_return_ HRESULT GetItems(_Out_opt_ UINT* pItemsCount, _Outptr_opt_result_maybenull_ wfc::IVector<IInspectable*>** ppItems);
    _Check_return_ HRESULT GiveFocusTo(_In_ xaml_controls::IControl* target, _In_ xaml::FocusState focusState);
    template<typename T>
    _Check_return_ HRESULT GiveFocusTo(_In_ T* target, _In_ xaml::FocusState focusState)
    {
        static_assert(!std::is_same<T, xaml_controls::IControl>::value, "Templated GiveFocusTo should not be called.");
        wrl::ComPtr<xaml_controls::IControl> targetAsControl;
        IFC_RETURN(target->QueryInterface(__uuidof(xaml_controls::IControl), &targetAsControl));
        return GiveFocusTo(targetAsControl.Get(), focusState);
    }
    _Check_return_ HRESULT GiveKeyboardFocusTo(_In_ xaml_controls::IControl* target);
    template<typename T>
    _Check_return_ HRESULT GiveKeyboardFocusTo(_In_ T* target)
    {
        static_assert(!std::is_same<T, xaml_controls::IControl>::value, "Templated GiveKeyboardFocusTo should not be called.");
        wrl::ComPtr<xaml_controls::IControl> targetAsControl;
        IFC_RETURN(target->QueryInterface(__uuidof(xaml_controls::IControl), &targetAsControl));
        return GiveKeyboardFocusTo(targetAsControl.Get());
    }
    _Check_return_ HRESULT PerformAutoFocus(_In_ xaml_input::FocusNavigationDirection direction);
    _Check_return_ HRESULT HasFocus(_Out_ bool* hasFocus);
    _Check_return_ HRESULT HasFocus(_In_ xaml::IDependencyObject* parent, _Out_ bool* hasFocus);
    template<typename T>
    _Check_return_ HRESULT HasFocus(_In_ T* parent, _Out_ bool* hasFocus)
    {
        static_assert(!std::is_same<T, xaml::IDependencyObject>::value, "Templated HasFocus should not be called.");
        wrl::ComPtr<xaml::IDependencyObject> parentAsDO;
        IFC_RETURN(parent->QueryInterface(__uuidof(xaml::IDependencyObject), &parentAsDO));
        return HasFocus(parentAsDO.Get(), hasFocus);
    }
    _Check_return_ HRESULT IsAscendantOfTarget(_In_ xaml::IDependencyObject* parent, _In_ xaml::IDependencyObject* child, _Out_ bool* isChildOfTarget);
    _Check_return_ HRESULT ValidateItemIndex();
    void PhysicalPixelsToDips(_In_ wf::Rect* physicalPixels, _Out_ wf::Rect* dipPixels);
#pragma endregion

#pragma region StateMachineCallbacks
    bool IPivotStateMachineCallbacks::IsHeaderItemsCarouselEnabled() const override { return m_isHeaderItemsCarouselEnabled; }
    unsigned IPivotStateMachineCallbacks::GetPivotPanelMultiplier() const override { return GetPivotPanelMultiplierImpl(); }
    _Check_return_ HRESULT SetPivotSectionOffset(_In_ DOUBLE offset) override;
    _Check_return_ HRESULT SetViewportOffset(_In_ DOUBLE offset, _In_ BOOLEAN animate, _Out_ bool *success) override;
    _Check_return_ HRESULT SetPivotSectionWidth(_In_ FLOAT width) override;
    _Check_return_ HRESULT SetSelectedIndex(_In_ INT32 idx, _In_ BOOLEAN updateVisual, _In_ BOOLEAN updateIndex, _In_ BOOLEAN updateItem, _In_ PivotAnimationDirection animationHint) override;
    _Check_return_ HRESULT StartFlyOutAnimation(_In_ DOUBLE from, _In_ DOUBLE headerOffset, _In_ bool toLeft) override;
    _Check_return_ HRESULT StartFlyInAnimation(_In_ DOUBLE to, _In_ bool fromLeft) override;
    _Check_return_ HRESULT SetParallaxRelationship(double sectionOffset, double sectionWidth, float viewportSize) override;
    _Check_return_ HRESULT SetSnappingBehavior(_In_ BOOLEAN single) override;
    _Check_return_ HRESULT SetViewportEnabled(_In_ BOOLEAN enabled) override;
    _Check_return_ HRESULT GetIsInDManipAnimation(_Out_ bool *isInDManipAnimation) override;
    _Check_return_ HRESULT CancelDManipAnimations() override;
    _Check_return_ HRESULT UpdateScrollViewerDragDirection(_In_ PivotAnimationDirection direction) override;
    _Check_return_ HRESULT UpdateFocusFollower() override;
#pragma endregion

#pragma region AnimatorCallbacks
    _Check_return_ HRESULT OnAnimationComplete() override;
#pragma endregion

#pragma region HeaderManagerCallbacks
public:
    _Check_return_ HRESULT OnHeaderItemTapped(_In_ INT newSelectedIndex, _In_ bool shouldPlaySound) override;
private:
    bool IPivotHeaderManagerCallbacks::IsHeaderItemsCarouselEnabled() const override { return m_isHeaderItemsCarouselEnabled; }
    unsigned IPivotHeaderManagerCallbacks::GetPivotPanelMultiplier() const override { return GetPivotPanelMultiplierImpl(); }
    _Check_return_ HRESULT GetHeaderTemplate(_Outptr_result_maybenull_ xaml::IDataTemplate** ppTemplate) override;
    _Check_return_ HRESULT OnHeaderPanelMeasure(float viewportSize) override;
#pragma endregion

#pragma region Event Firing Helpers
    _Check_return_ HRESULT RaiseOnPivotItemLoaded(_In_ xaml_controls::IPivotItem* pItem);
    _Check_return_ HRESULT RaiseOnPivotItemUnloaded(_In_ xaml_controls::IPivotItem* pItem);
    _Check_return_ HRESULT RaiseOnSelectionChanged(_In_ IInspectable* pOldItem, _In_ IInspectable* pNewItem);
#pragma endregion

#pragma region Event Handlers
public:
    _Check_return_ HRESULT OnPropertyChanged(
        _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);
    _Check_return_ HRESULT ValidateSelectedItem(_In_ IInspectable* pNewItem, _Out_ INT32* itemIdx, _Out_ BOOLEAN* isValid);
private:
    _Check_return_ HRESULT OnSelectedIndexChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);
    _Check_return_ HRESULT OnSelectedItemChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);
    _Check_return_ HRESULT OnIsLockedChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);
    _Check_return_ HRESULT OnHeaderFocusVisualPlacementChanged();
    _Check_return_ HRESULT OnIsHeaderItemsCarouselEnabledChanged();

    _Check_return_ HRESULT ValidateSelectedIndex(_In_ INT32 newIdx, _Out_ BOOLEAN* isValid);
    _Check_return_ HRESULT SyncItemToIndex(_In_ INT32 newIdx);

    _Check_return_ HRESULT UpdateTitleControlVisibility();

    _Check_return_ HRESULT OnViewChanged(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pEventArgs);
    _Check_return_ HRESULT OnViewChanging(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangingEventArgs* pEventArgs);
    _Check_return_ HRESULT OnScrollViewerLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);
    _Check_return_ HRESULT OnDirectManipulationStarted(_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2);
    _Check_return_ HRESULT OnDirectManipulationCompleted(_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2);

    _Check_return_ HRESULT OnHeaderGotFocus(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args);
    _Check_return_ HRESULT OnHeaderLostFocus(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args);
    _Check_return_ HRESULT UpdateHeaderManagerFocusState();
    _Check_return_ HRESULT OnHeaderKeyDown(_In_ IInspectable* sender, _In_ xaml_input::IKeyRoutedEventArgs* args);
    _Check_return_ HRESULT OnLayoutElementKeyDown(_In_ IInspectable* sender, _In_ xaml_input::IKeyRoutedEventArgs* args);

    _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

    _Check_return_ HRESULT OnSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);
#pragma endregion

#pragma region Event Handlers Helpers
    _Check_return_ HRESULT OnDirectManipulationWork(bool dmanipInProgress);
#pragma endregion

#pragma region LayoutBoundsMargin Adjustment
    // LayoutBoundsMargin adjustments consists of two parts:
    // - When the window changes size in portrait mode we apply a negative margin to account
    //   for the app bar.
    // - On display orientation changes we transition to a custom visual state to allow for
    //   different margins on different sides of the control.
    _Check_return_ HRESULT GoToOrientationState();

    static _Check_return_ HRESULT GetOrientation(_Out_ wgrd::DisplayOrientations* pOrientation);
#pragma endregion

    _Check_return_ HRESULT UnregisterNavigationButtonEvents();

    _Check_return_ HRESULT UpdateHeaderState();
    _Check_return_ HRESULT UpdateVisualStates();

    _Check_return_ HRESULT InitializeVisualStateInterfaces();

    _Check_return_ HRESULT MoveToPreviousItem(_In_ bool shouldWrap, _Out_opt_ bool *selectedIndexMoved = nullptr);
    _Check_return_ HRESULT MoveToNextItem(_In_ bool shouldWrap, _Out_opt_ bool *selectedIndexMoved = nullptr);

    _Check_return_ HRESULT MoveToFirstItem(_Out_opt_ bool *selectedIndexMoved = nullptr);
    _Check_return_ HRESULT MoveToLastItem(_Out_opt_ bool *selectedIndexMoved = nullptr);

    _Check_return_ HRESULT MoveToItem(_In_ INT newIndex, _In_ xaml::ElementSoundKind soundToPlay, _Out_opt_ bool *selectedIndexMoved = nullptr);

    bool ShouldWrap() const;
    _Check_return_ HRESULT ShouldUseStaticHeaders(_Out_ bool* result);
    _Check_return_ HRESULT GetHeaderAt(_In_ INT index, _Outptr_result_maybenull_ xaml::IUIElement** headerNoRef);

#pragma region Helpers
    _Check_return_ HRESULT InvalidateMeasure();
    _Check_return_ HRESULT InvalidateArrange();
    _Check_return_ HRESULT IsPointerTouchPointer(
        _In_ xaml_input::IPointerRoutedEventArgs* pointerEventArgs,
        _Out_ bool* isPointerTouchPointer);
    _Check_return_ HRESULT IsLoaded(_Out_ bool* isLoaded);
#pragma endregion

    Private::TrackerPtr<xaml_controls::IItemsPresenter> m_tpItemsPresenter;
    Private::TrackerPtr<xaml_controls::IContentControl> m_titleControl;
    Private::TrackerPtr<xaml_primitives::IPivotHeaderPanel> m_tpHeaderPanel;
    Private::TrackerPtr<xaml_primitives::IPivotHeaderPanel> m_tpStaticHeaderPanel;
    Private::TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;
    Private::TrackerPtr<xaml_primitives::IPivotPanel> m_tpPanel;
    Private::TrackerPtr<xaml_controls::IPanel> m_tpItemsPanel;
    Private::TrackerPtr<xaml_media::ITranslateTransform> m_tpItemsPanelTranslateTransform;
    Private::TrackerPtr<xaml_media::ICompositeTransform> m_tpItemsPanelCompositeTransform;
    Private::TrackerPtr<xaml_media::ICompositeTransform> m_tpHeaderTransform;
    Private::TrackerPtr<xaml_media::ICompositeTransform> m_tpStaticHeaderTransform;
    Private::TrackerPtr<xaml_controls::IButton> m_tpNextButton;
    Private::TrackerPtr<xaml_controls::IButton> m_tpPreviousButton;
    Private::TrackerPtr<xaml::IUIElement> m_tpLayoutElement;
    Private::TrackerPtr<xaml_media::ICompositeTransform> m_tpLayoutElementTransform;
    Private::TrackerPtr<xaml::IFrameworkElement> m_tpHeaderClipper;
    Private::TrackerPtr<xaml_media::IRectangleGeometry> m_tpHeaderClipperGeometry;
    Private::TrackerPtr<xaml::IFrameworkElement> m_tpLeftHeaderPresenter;
    Private::TrackerPtr<xaml::IFrameworkElement> m_tpRightHeaderPresenter;
    Private::TrackerPtr<xaml::IFrameworkElement> m_tpFocusFollower;
    Private::TrackerPtr<WUComp::IExpressionAnimation> m_tpFocusFollowerExpresssionAnimation;

    wrl::ComPtr<xaml::IVisualStateManagerStatics> m_spVSMStatics;

    xaml::Controls::IControl *m_pThisAsControlNoRef;

    EventRegistrationToken m_viewChangingToken;
    EventRegistrationToken m_viewChangedToken;
    EventRegistrationToken m_sizeChangedToken;
    EventRegistrationToken m_directManipulationStartedToken;
    EventRegistrationToken m_directManipulationCompletedToken;
    EventRegistrationToken m_scrollViewerLoadedToken;
    EventRegistrationToken m_headerGotFocusToken;
    EventRegistrationToken m_headerLostFocusToken;
    EventRegistrationToken m_headerKeyDownToken;
    EventRegistrationToken m_layoutElementKeyDownToken;
    EventRegistrationToken m_pointerEnteredHeaderToken;
    EventRegistrationToken m_pointerExitedHeaderToken;
    EventRegistrationToken m_pointerEnteredPreviousButtonToken;
    EventRegistrationToken m_pointerExitedPreviousButtonToken;
    EventRegistrationToken m_pointerEnteredNextButtonToken;
    EventRegistrationToken m_pointerExitedNextButtonToken;
    EventRegistrationToken m_previousButtonClickedToken;
    EventRegistrationToken m_nextButtonClickedToken;

    bool HasValidTemplate() const
    {
        return HasValidBlueTemplate() || HasValidThresholdTemplate();
    }

    // Returns whether the current control template is a valid windows 8.1 template.
    bool HasValidBlueTemplate() const
    {
        return m_tpItemsPresenter &&
               m_tpHeaderPanel &&
               m_tpHeaderTransform &&
               m_tpScrollViewer &&
               m_tpPanel &&
               m_tpItemsPanelTranslateTransform;
    }

    // Returns whether the current control template is a valid windows 10 template.
    bool HasValidThresholdTemplate() const
    {
        return m_tpItemsPresenter &&
               ((m_tpHeaderPanel && m_tpHeaderTransform && m_tpHeaderClipper && m_tpHeaderClipperGeometry) || m_tpStaticHeaderPanel) &&
               m_tpScrollViewer &&
               m_tpPanel &&
               m_tpItemsPanelTranslateTransform &&
               m_tpItemsPanelCompositeTransform &&
               m_tpLayoutElement &&
               m_tpLayoutElementTransform;
    }

    // These classes contain almost all of Pivot's state. All interactions
    // that deal with Viewport state should be pushed through PivotStateMachine
    // to ensure consistency. Since the synchronization and display of headers
    // is somewhat orthogonal to the primary movement of Pivot sections this
    // logic has been separated out into PivotHeaderManager, with callbacks
    // from PivotStateMachine, through Pivot, to PivotHeaderManager to synchronize
    // when needed.
    PivotStateMachine m_stateMachine;
    PivotAnimator m_animator;
    PivotHeaderManager m_headerManager;

    // We keep track of where item/index change events originate
    // from to reuse the same methods but still correctly fire
    // item/index change events.
    BOOLEAN m_fItemChangeReentryGuard;
    BOOLEAN m_fIndexChangeReentryGuard;

    // Used to store the old item when firing the selection changed event
    // to build event args.
    Private::TrackerPtr<IInspectable> m_tpOldItem;

    // Used to store the old PivotItem, used when firing the Unloaded
    // event to build event args.
    // NOTE: This is different than the item stored above. Once the
    // collection changes ItemsContainerGenerator won't map between
    // item and container if the item is no longe present in the collection
    // so we store both.
    Private::TrackerPtr<xaml_controls::IPivotItem> m_tpCurrentPivotItem;

    // Used to fire correct automation collection changed events.
    UINT m_automationItemCount;
    BOOLEAN m_automationIsLocked;
    INT m_automationSelectedIndex;

    // ChangeViewWithOptionalAnimation needs to be called right
    // before we submit the render in NWDrawTree, else the ZoomToRect
    // Dmanip operation will be mostly complete.
    EventRegistrationToken m_changeViewPreRenderToken;

    // When an item is first set from Collapsed->Visible it hasn't
    // had its template expanded. We subscribe to the 'Rendering' event
    // which occurs after the last layout pass to still fire off
    // the correct animation. We store the PivotItem of interest
    // in this pointer.
    Private::TrackerPtr<xaml_controls::IPivotItem> m_tpPendingSlideInItem;
    EventRegistrationToken m_slideInPreRenderToken;

    bool m_usingStaticHeaders;
    bool m_isControlKeyPressed;
    bool m_isShiftKeyPressed;
    bool m_isDirectManipulationInProgress{ false };
    bool m_isMouseOrPenPointerOverHeaders;
    bool m_isHeaderItemsCarouselEnabled;    // Shadows the value of Pivot.IsHeaderItemsCarouselEnabled
                                            // to make the callback APIs cleaner.

    enum class NavigationButtonsStates
    {
        NavigationButtonsHidden,
        NavigationButtonsVisible,   // Both are visible at the same time
        PreviousButtonVisible,
        NextButtonVisible
    } m_navigationButtonsState;

    // When true, instructs Pivot to give keyboard focus to the pivot item we are
    // in the process of selecting.
    bool m_keyboardFocusToNextPivotItemPending;

    wf::Rect m_cachedSelectedItemHeaderBoundingRectangle;

    INT m_previousSelectedIndex;
    unsigned m_itemCount;
    PivotAnimationDirection m_pivotDragDirection;

protected:
    virtual ~Pivot() override;

#pragma region FactoryCaches
public:
    // Caches statics and factories used during Pivot lifetime.
    _Check_return_ static HRESULT EnsureStaticsAndFactories();

    // Cached static interfaces.
    static wrl::ComPtr<xaml_animation::IStoryboardStatics> s_spStoryboardStatics;
    static wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> s_spVisualTreeHelperStatics;
    static wrl::ComPtr<xaml_media::ICompositionTargetStatics> s_spCompositionTargetStatics;
#pragma endregion
};

ActivatableClassWithFactory(Pivot, PivotFactory);

} } } } XAML_ABI_NAMESPACE_END