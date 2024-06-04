// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This file is no longer generated, so it is fine to edit by hand.

#pragma once

#include <FeatureFlags.h>
#include <internaleventhandler.h>

class CAutomationPeer;
class CBitmapSource;
class CButton;
class CContentRoot;
class CControl;
class CDependencyObject;
class CDependencyProperty;
class CDragEventArgs;
class CEventArgs;
class CFlyoutBase;
class CFrameworkElement;
class CItemCollection;
class CItemsControl;
class CItemsPresenter;
class CKeyboardAccelerator;
class CMenuFlyout;
class CPasswordBox;
class CPointerEventArgs;
class CRoutedEventArgs;
class CStoryboard;
class CStyle;
class CTextBlock;
class CTextBox;
class CUIElement;
class CXamlIslandRoot;
class VisualStateToken;
class VisualTree;
class XamlServiceProviderContext;
enum class DeferredElementStateChange;
enum class KnownEventIndex : UINT16;
enum PixelFormat;
namespace Automation { class CValue; }
namespace DirectUI { enum class ApplicationHighContrastAdjustment; }
namespace DirectUI { enum class ElementSoundKind : uint8_t; }
namespace DirectUI { enum class FocusVisualKind : uint8_t; }
namespace DirectUI { enum class InputPaneState : uint8_t; }
namespace DirectUI { enum class ManagedEvent : uint8_t; }
namespace DirectUI { enum class TransitionParent : uint8_t; }
namespace DirectUI { enum EReferenceTrackerWalkType; }
namespace UIAXcp { enum APAutomationProperties; }
namespace UIAXcp { enum APPatternInterface; }
namespace UIAXcp { enum AutomationNavigationDirection; }
struct PropertyChangedParams;
struct XamlPropertyToken;
struct XamlQualifiedObject;
struct XamlTypeToken;

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft::Windows::ApplicationModel::Resources {
    struct IResourceManager;
}
XAML_ABI_NAMESPACE_END

namespace FxCallbacks
{
    _Check_return_ HRESULT DependencyObject_NotifyPropertyChanged(_In_ CDependencyObject* pDO, _In_ const PropertyChangedParams& args);

    _Check_return_ HRESULT DependencyObject_EnterImpl(_In_ CDependencyObject* nativeDO, _In_ CDependencyObject* nativeNamescopeOwner, _In_ bool bLive, _In_ bool bSkipNameRegistration, _In_ bool bCoercedIsEnabled, _In_ bool bUseLayoutRounding);

    _Check_return_ HRESULT DependencyObject_LeaveImpl(_In_ CDependencyObject* nativeDO, _In_ CDependencyObject* nativeNamescopeOwner, _In_ bool bLive, _In_ bool bSkipNameRegistration, _In_ bool bCoercedIsEnabled, _In_ bool bVisualTreeBeingReset);

    _Check_return_ HRESULT DependencyObject_SetBinding(_In_ CDependencyObject* nativeTarget, _In_ KnownPropertyIndex propertyId, _In_ CDependencyObject* pBinding);

    _Check_return_ HRESULT DependencyObject_SetPeerReferenceToProperty(_In_ CDependencyObject* nativeTarget, _In_ const CDependencyProperty* pDP, _In_ const CValue& value, _In_ bool bPreservePegNoRef= false, _In_opt_ IInspectable* pNewValueOuter= nullptr, _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter= nullptr);

    _Check_return_ HRESULT DependencyObject_AddPeerReferenceToItem(_In_ CDependencyObject* nativeOwner, _In_ CDependencyObject* nativeTarget);

    _Check_return_ HRESULT DependencyObject_RemovePeerReferenceToItem(_In_ CDependencyObject* nativeOwner, _In_ CDependencyObject* nativeTarget);

    _Check_return_ HRESULT DependencyObject_OnCollectionChanged(_In_ CDependencyObject* nativeOwner, _In_ XUINT32 nChangeType, _In_ XUINT32 nIndex);

    _Check_return_ HRESULT DependencyObject_GetDefaultValue(_In_ CDependencyObject* pReferenceObject, _In_ const CDependencyProperty* pDP, _Out_ CValue* pValue);

    _Check_return_ HRESULT DependencyObject_NotifyDeferredElementStateChanged(_In_ CDependencyObject* parent, _In_ KnownPropertyIndex propertyIndex, _In_ DeferredElementStateChange state, _In_ UINT32 collectionIndex, _In_ CDependencyObject* realizedElement);

    _Check_return_ HRESULT DependencyObject_RefreshExpressionsOnThemeChange(_In_ CDependencyObject* nativeDO, _In_ Theming::Theme theme, _In_ bool forceRefresh);

    _Check_return_ HRESULT FlyoutPresenter_GetParentMenuFlyoutSubItem(_In_ CDependencyObject* nativeDO, _Outptr_ CDependencyObject** ppMenuFlyoutSubItem);

    _Check_return_ HRESULT UIElement_OnCreateAutomationPeer(_In_ CDependencyObject* nativeTarget, _Out_ CAutomationPeer** returnAP);

    _Check_return_ HRESULT UIElement_GetCanManipulateElements(_In_ CUIElement* nativeTarget, _Out_ bool* fCanManipulateElementsByTouch, _Out_ bool* fCanManipulateElementsNonTouch, _Out_ bool* fCanManipulateElementsWithBringIntoViewport);

    _Check_return_ HRESULT UIElement_SetManipulationHandler(_In_ CUIElement* nativeTarget, _In_opt_ void* nativeManipulationHandler);

    _Check_return_ HRESULT UIElement_SetManipulationHandlerWantsNotifications(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fWantsNotifications);

    _Check_return_ HRESULT UIElement_SetPointedElement(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativePointedElement);

    _Check_return_ HRESULT UIElement_GetManipulatedElement(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativePointedElement, _In_opt_ CUIElement* nativeChildElement, _Out_ CUIElement** nativeManipulatedElement);

    _Check_return_ HRESULT UIElement_GetManipulationViewport(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _Out_opt_ XRECTF* pBounds, _Out_opt_ CMILMatrix* pInputTransform, _Out_opt_ XUINT32* pTouchConfiguration, _Out_opt_ XUINT32* pNonTouchConfiguration, _Out_opt_ XUINT32* pBringIntoViewportConfiguration, _Out_opt_ XUINT32* pHorizontalOverpanMode, _Out_opt_ XUINT32* pVerticalOverpanMode, _Out_opt_ XUINT8* pcConfigurations, _Outptr_result_buffer_maybenull_(* pcConfigurations) XUINT32** ppConfigurations, _Out_opt_ XUINT32* pChainedMotionTypes);

    _Check_return_ HRESULT UIElement_GetManipulationPrimaryContent(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _Out_opt_ XSIZEF* pOffsets, _Out_opt_ XRECTF* pBounds, _Out_opt_ XUINT32* pHorizontalAlignment, _Out_opt_ XUINT32* pVerticalAlignment, _Out_opt_ XFLOAT* pMinZoomFactor, _Out_opt_ XFLOAT* pMaxZoomFactor, _Out_opt_ bool* pfIsHorizontalStretchAlignmentTreatedAsNear, _Out_opt_ bool* pfIsVerticalStretchAlignmentTreatedAsNear, _Out_opt_ bool* pfIsLayoutRefreshed);

    _Check_return_ HRESULT UIElement_GetManipulationSecondaryContent(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeContentElement, _Out_ XSIZEF* pOffsets);

    _Check_return_ HRESULT UIElement_GetManipulationPrimaryContentTransform(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fInManipulation, _In_ bool fForInitialTransformationAdjustment, _In_ bool fForMargins, _Out_opt_ XFLOAT* pTranslationX, _Out_opt_ XFLOAT* pTranslationY, _Out_opt_ XFLOAT* pZoomFactor);

    _Check_return_ HRESULT UIElement_GetManipulationSecondaryContentTransform(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeContentElement, _Out_ XFLOAT* pTranslationX, _Out_ XFLOAT* pTranslationY, _Out_ XFLOAT* pZoomFactor);

    _Check_return_ HRESULT UIElement_GetManipulationSnapPoints(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XUINT32 motionType, _Out_ bool* pfAreSnapPointsOptional, _Out_ bool* pfAreSnapPointsSingle, _Out_ bool* pfAreSnapPointsRegular, _Out_ XFLOAT* pRegularOffset, _Out_ XFLOAT* pRegularInterval, _Out_ XUINT32* pcIrregularSnapPoints, _Outptr_result_buffer_(* pcIrregularSnapPoints) XFLOAT** ppIrregularSnapPoints, _Out_ XUINT32* pSnapCoordinate);

    _Check_return_ HRESULT UIElement_NotifyManipulatabilityAffectingPropertyChanged(_In_ CUIElement* nativeTarget, _In_ bool fIsInLiveTree);

    _Check_return_ HRESULT UIElement_NotifyContentAlignmentAffectingPropertyChanged(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fIsForHorizontalAlignment, _In_ bool fIsForStretchAlignment, _In_ bool fIsStretchAlignmentTreatedAsNear);

    _Check_return_ HRESULT UIElement_NotifyManipulationProgress(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XUINT32 state, _In_ XFLOAT xCumulativeTranslation, _In_ XFLOAT yCumulativeTranslation, _In_ XFLOAT zCumulativeFactor, _In_ XFLOAT xInertiaEndTranslation, _In_ XFLOAT yInertiaEndTranslation, _In_ XFLOAT zInertiaEndFactor, _In_ XFLOAT xCenter, _In_ XFLOAT yCenter, _In_ bool fIsInertiaEndTransformValid, _In_ bool fIsInertial, _In_ bool fIsTouchConfigurationActivated, _In_ bool fIsBringIntoViewportConfigurationActivated);

    _Check_return_ HRESULT UIElement_NotifyManipulationStateChanged(_In_ CUIElement* nativeTarget, _In_ XUINT32 state);

    _Check_return_ HRESULT UIElement_IsBringIntoViewportNeeded(_In_ CUIElement* nativeTarget, _Out_ bool* bringIntoViewportNeeded);

    _Check_return_ HRESULT UIElement_NotifyBringIntoViewportNeeded(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XFLOAT translationX, _In_ XFLOAT translationY, _In_ XFLOAT zoomFactor, _In_ bool fTransformIsValid, _In_ bool fTransformIsInertiaEnd);

    _Check_return_ HRESULT UIElement_NotifySnapPointsChanged(_In_ CUIElement* nativeTarget, _In_ bool fHorizontalSnapPoints);

    _Check_return_ HRESULT UIElement_NotifyCanDragChanged(_In_ CUIElement* nativeTarget, _In_ bool fCanDrag);

    _Check_return_ HRESULT UIElement_OnDirectManipulationDraggingStarted(_In_ CUIElement* nativeTarget);

    _Check_return_ HRESULT UIElement_NotifyInputPaneStateChange(_In_ CUIElement* nativeTarget, _In_ DirectUI::InputPaneState inputPaneState, _In_ XRECTF inputPaneBounds);

    _Check_return_ HRESULT UIElement_ApplyInputPaneTransition(_In_ CUIElement* nativeTarget, _In_ bool fEnableThemeTransition);

    _Check_return_ HRESULT UIElement_ApplyElevationEffect(_In_ CUIElement* target, unsigned int depth = 0);

    _Check_return_ HRESULT UIElement_GetScrollContentPresenterViewportRatios(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativeChild, _Out_ XSIZEF* ratios);

    _Check_return_ HRESULT UIElement_IsScrollViewerContentScrollable(_In_ CUIElement* nativeTarget, _Out_ bool* isContentHorizontallyScrollable, _Out_ bool* isContentVerticallyScrollable);

    _Check_return_ HRESULT UIElement_ProcessTabStop(_In_ CContentRoot* contentRoot, _In_opt_ CDependencyObject* pFocusedElement, _In_opt_ CDependencyObject* pCandidateTabStopElement, const bool isShiftPressed, const bool didCycleFocusAtRootVisualScope, _Outptr_ CDependencyObject** ppNewTabStop, _Out_ bool* pIsTabStopOverrided);

    _Check_return_ HRESULT UIElement_GetNextTabStop(_In_ CDependencyObject* pFocusedElement, _Outptr_ CDependencyObject** ppNextTabStop);

    _Check_return_ HRESULT UIElement_GetPreviousTabStop(_In_ CDependencyObject* pFocusedElement, _Outptr_ CDependencyObject** ppPreviousTabStop);

    _Check_return_ HRESULT UIElement_GetFirstFocusableElement(_In_ CDependencyObject* pSearchStart, _Outptr_ CDependencyObject** ppFirstFocusable);

    _Check_return_ HRESULT UIElement_GetLastFocusableElement(_In_ CDependencyObject* pSearchStart, _Outptr_ CDependencyObject** ppLastFocusable);

    _Check_return_ HRESULT UIElement_GetDManipElementAndProperty(_In_ CUIElement* nativeTarget, _In_ KnownPropertyIndex targetProperty, _Outptr_ CDependencyObject** ppDManipElement, _Out_ XUINT32* pDManipProperty);

    _Check_return_ HRESULT UIElement_GetDManipElement(_In_ CUIElement* nativeTarget, _Outptr_ CDependencyObject** ppDManipElement);

    bool UIElement_ShouldPlayImplicitShowHideAnimation(_In_ CUIElement* nativeTarget);

    _Check_return_ HRESULT BitmapSource_SetSourceAsync(_In_ CBitmapSource* pNative, _In_ wsts::IRandomAccessStream* pStreamSource, _Outptr_ wf::IAsyncAction** ppReturnValue);

    _Check_return_ HRESULT FrameworkElement_MeasureOverride(_In_ CFrameworkElement* nativeTarget, _In_ XFLOAT inWidth, _In_ XFLOAT inHeight, _Out_ XFLOAT* outWidth, _Out_ XFLOAT* outHeight);

    _Check_return_ HRESULT FrameworkElement_ArrangeOverride(_In_ CFrameworkElement* nativeTarget, _In_ XFLOAT inWidth, _In_ XFLOAT inHeight, _Out_ XFLOAT* outWidth, _Out_ XFLOAT* outHeight);

    _Check_return_ HRESULT FrameworkElement_OnApplyTemplate(_In_ CFrameworkElement* nativeTarget);

    _Check_return_ HRESULT FrameworkElement_GetLogicalParentForAP(_In_ CDependencyObject* nativeTarget, _Outptr_ CDependencyObject** ppLogicalParentForAP);

    _Check_return_ HRESULT Hub_NotifyDMHubStateChange(_In_ CDependencyObject* nativeTarget, _In_ XUINT32 state);

    _Check_return_ HRESULT UserControl_RegisterAppBars(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT UserControl_UnregisterAppBars(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT Control_UpdateVisualState(_In_ CDependencyObject* nativeTarget, _In_ bool fUseTransitions);

    _Check_return_ HRESULT Control_UpdateEngagementState(_In_ CControl* nativeTarget, _In_ bool fEngage);

    _Check_return_ HRESULT Control_GetBuiltInStyle(_In_ CDependencyObject* nativeTarget, _Out_ CStyle** nativeStyle);

    _Check_return_ HRESULT Image_OnPlayToConnectionShutdown(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT MediaElement_OnMediaOpened(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT MediaElement_OnMediaFailed(_In_ CDependencyObject* nativeDO, _In_ XUINT32 errorCode);

    _Check_return_ HRESULT MediaElement_OnEnterFullWindowMode(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT MediaElement_OnExitFullWindowMode(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT MediaElement_OnReadyToLaunchMediaEngine(_In_ CDependencyObject* nativeDO);

    _Check_return_ HRESULT TextBox_NotifyOffsetsChanging(_In_ CDependencyObject* nativeTextBoxView, XDOUBLE oldHorizontalOffset, XDOUBLE newHorizontalOffset, XDOUBLE oldVerticalOffset, XDOUBLE newVerticalOffset);

    _Check_return_ HRESULT TextBox_InvalidateScrollInfo(_In_ CDependencyObject* nativeTextBoxView);

    _Check_return_ HRESULT TextBox_OnApplyTemplateHandler(_In_ CDependencyObject* nativeTextBox);

    _Check_return_ HRESULT TextBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativeTextBox, _In_ bool isEnabled);

    _Check_return_ HRESULT TextBox_GetBringIntoViewOnFocusChange(_In_ CDependencyObject* nativeTextBox, _Out_ bool* pBringIntoViewOnFocusChange);

    _Check_return_ HRESULT TextBox_OnTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ bool fTextChanged);

    _Check_return_ HRESULT TextBox_AddMenuFlyoutItemClickHandler(_In_ CDependencyObject* pMenuFlyoutItem, _In_ INTERNAL_EVENT_HANDLER eventHandler);

    _Check_return_ HRESULT TextBoxView_CaretChanged(_In_ CDependencyObject* pNativeTextBoxView);

    _Check_return_ HRESULT TextBoxView_CaretVisibilityChanged(_In_ CDependencyObject* pNativeTextBoxView);

    _Check_return_ HRESULT TextBoxView_InvalidateView(_In_ CDependencyObject* pNativeTextBoxView);

    _Check_return_ HRESULT TextBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeTextBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);

    _Check_return_ HRESULT PasswordBox_OnApplyTemplateHandler(_In_ CPasswordBox* nativePasswordBox);

    _Check_return_ HRESULT PasswordBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativePasswordBox, _In_ bool isEnabled);

    _Check_return_ HRESULT PasswordBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativePasswordBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);

    _Check_return_ HRESULT RichEditBox_OnApplyTemplateHandler(_In_ CDependencyObject* nativeRichEditBox);

    _Check_return_ HRESULT RichEditBox_OnTextChangingHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ bool fTextChanged);

    _Check_return_ HRESULT RichEditBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ bool isEnabled);

    _Check_return_ HRESULT RichEditBox_HandleHyperlinkNavigation(_In_reads_(cLinkText) WCHAR* pLinkText, XUINT32 cLinkText);

    _Check_return_ HRESULT RichEditBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);

    _Check_return_ HRESULT TextBlock_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);

    _Check_return_ HRESULT RichTextBlock_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeRichTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);

    _Check_return_ HRESULT VisualStateManager_CustomVSMGoToState(_In_ CDependencyObject* pControl, _In_ VisualStateToken token, _In_ int groupIndex, _In_ bool useTransitions, _Out_ bool* bSucceeded);

    _Check_return_ HRESULT ItemsControl_SetItemCollection(_In_ CItemCollection* nativeItemCollection, _In_ CItemsControl* nativeItemsControl);

    _Check_return_ HRESULT ItemsControl_ClearVisualChildren(_In_ CItemsControl* nativeItemsControl, _In_ bool bHostIsReplaced);

    _Check_return_ HRESULT ItemsControl_DisplayMemberPathChanged(_In_ CItemsControl* nativeItemsControl);

    _Check_return_ HRESULT ItemsControl_RecreateVisualChildren(_In_ CItemsControl* nativeItemsControl);

    _Check_return_ HRESULT ItemsControl_NotifyAllItemsAdded(_In_ CItemsControl* nativeItemsControl);

    _Check_return_ HRESULT ItemsPresenter_Dispose(_In_ CItemsPresenter* pNativeItemsPresenter);

    _Check_return_ HRESULT ContentControl_OnContentChanged(_In_ CDependencyObject* nativeTarget, _In_ CValue* oldContentValue, _In_ CValue* newContentValue, _In_opt_ IInspectable* pValueOuter);

    _Check_return_ HRESULT ContentPresenter_BindDefaultTextBlock(_In_ CTextBlock* pTextBlock, _In_opt_ const xstring_ptr* pstrBindingPath);

    _Check_return_ HRESULT ContentPresenter_OnChildrenCleared(_In_ CDependencyObject* nativeTarget);

    _Check_return_ HRESULT ContentPresenter_OnContentTemplateChanged(_In_ CDependencyObject* target, _In_ const PropertyChangedParams& args);

    _Check_return_ HRESULT ContentPresenter_OnContentTemplateSelectorChanged(_In_ CDependencyObject* target, _In_ const PropertyChangedParams& args);

    _Check_return_ HRESULT Popup_OnClosed(_In_ CDependencyObject* nativePopup);

    void Popup_OnIslandLostFocus(_In_ CDependencyObject* nativePopup);

    bool TextBlock_HasDataboundText(_In_ CTextBlock* nativeTextBlock);

    _Check_return_ HRESULT JoltHelper_FireEvent(_In_ CDependencyObject* pListener, _In_ KnownEventIndex eventId, _In_ CDependencyObject* pSender, _In_ CEventArgs* pArgs, _In_ XUINT32 flags);

    _Check_return_ HRESULT JoltHelper_RaiseEvent(_In_ CDependencyObject* target, _In_ DirectUI::ManagedEvent eventId, _In_ CEventArgs* coreEventArgs);

    _Check_return_ HRESULT DragDrop_PopulateDragEventArgs(_In_ CDragEventArgs* pArgs);

    _Check_return_ HRESULT DragDrop_CheckIfCustomVisualShouldBeCleared(_In_ CDependencyObject* pSource);
    bool HasDragDrop_CheckIfCustomVisualShouldBeCleared();

    _Check_return_ HRESULT RaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset(_In_ IInspectable* pOperation, _In_ CDependencyObject* pSource);
    bool HasRaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset();

    _Check_return_ HRESULT RaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource);
    bool HasRaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement();

    _Check_return_ HRESULT DragEventArgs_GetIsDeferred(_In_ CDragEventArgs* args, _Out_ bool* isDeferred);
    bool HasDragEventArgs_GetIsDeferred();

    _Check_return_ HRESULT JoltHelper_TriggerGCCollect();

    _Check_return_ HRESULT Error_ReportUnhandledError(_In_ XUINT32 hr);

    _Check_return_ HRESULT FrameworkApplication_GetApplicationHighContrastAdjustment(_Out_ DirectUI::ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment);

    _Check_return_ HRESULT FrameworkCallbacks_SetTemplateBinding(_In_ CDependencyObject* source, _In_ const CDependencyProperty* sourceProperty, _In_ CDependencyObject* target, _In_ const CDependencyProperty* targetProperty);

    _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeIDFromObject(_In_ CDependencyObject* target, _Out_ KnownTypeIndex* typeID);

    _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeFullName(_In_ CDependencyObject* target, _Out_ xstring_ptr* pstrFullName);

    _Check_return_ HRESULT FrameworkCallbacks_AreObjectsOfSameType(_In_ CDependencyObject* nativeObject1, _In_ CDependencyObject* nativeObject2, _Out_ bool* areEqual);

    _Check_return_ HRESULT FrameworkCallbacks_SetDataContext(_In_ CFrameworkElement* pElement, _In_ CValue* pValue);

    _Check_return_ HRESULT FrameworkCallbacks_ClearDataContext(_In_ CFrameworkElement* pElement);

    _Check_return_ HRESULT FrameworkCallbacks_SupportInitializeEndInit(_In_ CDependencyObject* nativeTarget, _In_ const std::shared_ptr<XamlServiceProviderContext>& context);

    _Check_return_ HRESULT FrameworkCallbacks_CheckPeerType(_In_ CDependencyObject* nativeRoot, _In_ const xstring_ptr& strPeerType, _In_ XINT32 bCheckExact);

    _Check_return_ HRESULT FrameworkCallbacks_OnParentUpdated(_In_ CDependencyObject* childElement, _In_opt_ CDependencyObject* oldParentElement, _In_opt_ CDependencyObject* newParentElement, _In_ bool isNewParentAlive);

    _Check_return_ HRESULT FrameworkCallbacks_PropagateDataContextChange(_In_ CFrameworkElement* element);

    _Check_return_ HRESULT FrameworkCallbacks_CreateManagedPeer(_In_ CDependencyObject* element, _In_ KnownTypeIndex typeIndex, _In_ bool fPeggedNoRef, _In_ bool fPeggedRef, _In_ bool isShutdownException);

    void FrameworkCallbacks_UnpegManagedPeerNoRef(_In_ CDependencyObject* element);

    void FrameworkCallbacks_PegManagedPeerNoRef(_In_ CDependencyObject* element);

    void FrameworkCallbacks_TryPegPeer(_In_ CDependencyObject* element, _Out_ bool* pPegged, _Out_ bool* pIsPendingDelete);

    _Check_return_ HRESULT FrameworkCallbacks_ReferenceTrackerWalk(_In_ CDependencyObject* element, _In_ DirectUI::EReferenceTrackerWalkType walkType, _In_ bool isRoot, _Out_ bool* pIsPeerAlive, _Out_ bool* pWalked);

    _Check_return_ HRESULT FrameworkCallbacks_SetExpectedReferenceOnPeer(_In_ CDependencyObject* element);

    _Check_return_ HRESULT FrameworkCallbacks_ClearExpectedReferenceOnPeer(_In_ CDependencyObject* element);

    _Check_return_ HRESULT FrameworkCallbacks_Hyperlink_OnClick(_In_ CDependencyObject* nativeHost);

    _Check_return_ HRESULT FrameworkCallbacks_LoadThemeResources();

    _Check_return_ HRESULT FrameworkCallbacks_BudgetService_StoreFrameTime(_In_ bool isBeginningOfTick);

    _Check_return_ HRESULT FrameworkCallbacks_IsAnimationEnabled(_Out_ bool* pIsAnimationEnabled);

    _Check_return_ HRESULT FrameworkCallbacks_PhasedWorkDistributor_PerformWork(_Out_ bool* pWorkRemaining);

    _Check_return_ HRESULT FrameworkCallbacks_IsDXamlCoreShuttingDown(_Out_ bool* pIsDXamlCoreShuttingDown);

    _Check_return_ HRESULT XcpImports_ParticipateInTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XINT32 transitionTrigger, _Out_ bool* DoesParticipate);

    _Check_return_ HRESULT XcpImports_CreateStoryboardsForTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XRECTF startBounds, _In_ XRECTF destinationBounds, _In_ XINT32 transitionTrigger, _Out_ XINT32* cStoryboards, _Outptr_result_buffer_(* cStoryboards) CStoryboard*** pppTransitionStoryboard, _Out_ DirectUI::TransitionParent* parentToTransitionEnum);

    _Check_return_ HRESULT XcpImports_NotifyLayoutTransitionStart(_In_ CDependencyObject* nativeUIElement);

    _Check_return_ HRESULT XcpImports_NotifyLayoutTransitionEnd(_In_ CDependencyObject* nativeUIElement);

    _Check_return_ HRESULT XcpImports_StaggerManaged(_In_ CDependencyObject* nativeStaggerFunction, _In_ XINT32 cElements, _In_reads_(cElements) CUIElement** ppElements, _In_reads_(cElements) XRECTF* pBounds, _Out_writes_(cElements) XFLOAT* pDelays);

    _Check_return_ HRESULT XcpImports_GetDynamicTimelines(_In_ CDependencyObject* nativeDynamicTimeline, _In_ bool bGenerateSteadyStateOnly, _In_ CValue* timelineCollection);

    void XcpImports_PerFrameCallback();

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerStringValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Out_writes_z_(*cString) WCHAR* pString, _Inout_ XINT32* cString);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerIntValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ XINT32* nRetVal);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerPointValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Out_ XPOINTF* pPointF);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerRectValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ XRECTF* pRectF);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerAPValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ CDependencyObject** returnAP);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerDOValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ CDependencyObject** returnDO);

    _Check_return_ HRESULT AutomationPeer_CallAutomationPeerMethod(_In_ CDependencyObject* nativeTarget, _In_ XINT32 methodIndex);

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerChildren(_In_ CDependencyObject* nativeTarget, _In_ XUINT32 methodIndex, _Inout_ XINT32* returnCount, __deref_inout_ecount(* returnCount) CDependencyObject*** returnAP);

    _Check_return_ HRESULT AutomationPeer_Navigate(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

    _Check_return_ HRESULT AutomationPeer_GetElementFromPoint(_In_ CDependencyObject* nativeTarget, _In_ CValue param, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

    _Check_return_ HRESULT AutomationPeer_GetFocusedElement(_In_ CDependencyObject* nativeTarget, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

    _Check_return_ HRESULT AutomationPeer_GetPattern(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject** nativeInterface, _In_ UIAXcp::APPatternInterface eInterface);

    _Check_return_ HRESULT AutomationPeer_UIATextRangeInvoke(_In_ CDependencyObject* nativeTarget, _In_ XINT32 eFunction, _In_ XINT32 cParams, _In_opt_ void* pvParams, _In_opt_ Automation::CValue* pRetVal);

    _Check_return_ HRESULT AutomationPeer_UIAPatternInvoke(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APPatternInterface eInterface, _In_ XINT32 eFunction, _In_ XINT32 cParams, _In_opt_ void* pvParams, _In_opt_ Automation::CValue* pRetVal);

    _Check_return_ HRESULT AutomationPeer_NotifyNoUIAClientObjectForAP(_In_ CDependencyObject* nativeTarget);

    _Check_return_ HRESULT AutomationPeer_GenerateAutomationPeerEventsSource(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject* nativeTargetParent);

    _Check_return_ HRESULT Virtualization_ExecuteDeferredUnlinkAction(_In_ CUIElement* nativeTarget);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateInstance(_In_ XamlTypeToken inXamlType, _Out_ XamlQualifiedObject* newObject);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateFromValue(_In_ void* inServiceContext, _In_ XamlTypeToken inTs, _In_ XamlQualifiedObject* qoValue, _In_ XamlPropertyToken inProperty, _In_ XamlQualifiedObject* qoRootInstance, _Out_ XamlQualifiedObject* qo);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetValue(_In_ const XamlQualifiedObject* qoObj, _In_ XamlPropertyToken inProperty, _Out_ XamlQualifiedObject* outValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetAmbientValue(_In_ const XamlQualifiedObject* qoObj, _In_ XamlPropertyToken inProperty, _Out_ XamlQualifiedObject* outValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetValue(_In_ XamlQualifiedObject* inObj, _In_ XamlPropertyToken inProperty, _In_ XamlQualifiedObject* inValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_Add(_In_ XamlQualifiedObject* qoCollection, _In_ XamlQualifiedObject* inValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_AddToDictionary(_In_ XamlQualifiedObject* dictionary, _In_ XamlQualifiedObject* inKey, _In_ XamlQualifiedObject* inValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CallProvideValue(_In_ XamlQualifiedObject* markupExtension, _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext, _Out_ XamlQualifiedObject* outValue);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetConnectionId(_In_ XamlQualifiedObject* qoComponentConnector, _In_ XamlQualifiedObject* qoConnectionId, _In_ XamlQualifiedObject* qoTarget);

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetXBindConnector(_In_ XamlQualifiedObject* qoComponentConnector, _In_ XamlQualifiedObject* qoConnectionId, _In_ XamlQualifiedObject* qoTarget, _In_ XamlQualifiedObject* qoReturnConnector);

    _Check_return_ HRESULT ApplicationBarService_GetAppBarStatus(_In_ CDependencyObject* object, _Out_ bool* pbIsOpenTop, _Out_ bool* pbIsStickyTop, _Out_ XFLOAT* pWidthTop, _Out_ XFLOAT* pHeightTop, _Out_ bool* pbIsOpenBottom, _Out_ bool* pbIsStickyBottom, _Out_ XFLOAT* pWidthBottom, _Out_ XFLOAT* pHeightBottom);

    _Check_return_ HRESULT ApplicationBarService_ProcessToggleApplicationBarsFromMouseRightTapped(_In_ IInspectable* xamlRootInspectable);

    _Check_return_ HRESULT Window_GetContentRootBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds);

    _Check_return_ HRESULT Window_GetContentRootLayoutBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds);

    _Check_return_ HRESULT Window_GetRootScrollViewer(_Outptr_ CDependencyObject** ppRootScrollViewer);

    bool Window_AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat);

    _Check_return_ HRESULT TextElement_OnCreateAutomationPeer(_In_ CDependencyObject* nativeTarget, _Out_ CAutomationPeer** returnAP);

    _Check_return_ HRESULT CommandBarElementCollection_ValidateItem(_In_ CDependencyObject* pObject);

    _Check_return_ HRESULT HubSectionCollection_ValidateItem(_In_ CDependencyObject* pObject);

    _Check_return_ HRESULT MenuFlyoutItemBaseCollection_ValidateItem(_In_ CDependencyObject* pObject);

    _Check_return_ HRESULT FlyoutBase_IsOpen(_In_ CFlyoutBase* flyoutBase, _Out_ bool& isOpen);

    _Check_return_ HRESULT FlyoutBase_ShowAt(_In_ CFlyoutBase* pFlyoutBase, _In_ CFrameworkElement* pTarget);

    _Check_return_ HRESULT FlyoutBase_ShowAt(_In_ CFlyoutBase* pFlyoutBase, _In_ CFrameworkElement* pTarget, _In_ wf::Point point, _In_ wf::Rect exclusionRect, _In_ xaml_primitives::FlyoutShowMode flyoutShowMode);

    _Check_return_ HRESULT MenuFlyout_ShowAt(_In_ CMenuFlyout* pMenuFlyout, _In_ CUIElement* pTarget, _In_ wf::Point point);

    _Check_return_ HRESULT UIElement_IsDraggableOrPannable(_In_ CUIElement* pUIElement, _Out_ bool* pIsDraggableOrPannable);

    bool DXamlCore_IsWinRTDndOperationInProgress();

    bool DXamlCore_IsMapControl(_In_ CDependencyObject* pDO);

    _Check_return_ HRESULT FlyoutBase_CloseOpenFlyout(_In_opt_ CFlyoutBase* parentFlyout);

    _Check_return_ HRESULT FlyoutBase_OnClosing(_In_ CFlyoutBase* object, _Out_ bool* cancel);

    _Check_return_ HRESULT FlyoutBase_Hide(_In_opt_ CFlyoutBase* flyout);

    _Check_return_ HRESULT FlyoutBase_GetPlacementTargetNoRef(_In_ CFlyoutBase* flyout, _Outptr_ CFrameworkElement** placementTarget);

    _Check_return_ HRESULT Button_SuppressFlyoutOpening(_In_ CButton* button);

    _Check_return_ HRESULT ElementSoundPlayerService_RequestInteractionSoundForElement(_In_ DirectUI::ElementSoundKind sound, _In_ CDependencyObject* pControl);

    _Check_return_ HRESULT ElementSoundPlayerService_PlayInteractionSound();

    _Check_return_ HRESULT ExternalObjectReference_GetTarget(_In_ CDependencyObject* pDO, _Outptr_opt_result_maybenull_ IInspectable** ppTarget);

    _Check_return_ HRESULT ToolTipService_RegisterToolTip(_In_ CDependencyObject* owner, _In_ CFrameworkElement* container);

    _Check_return_ HRESULT ToolTipService_UnregisterToolTip(_In_ CDependencyObject* owner, _In_ CFrameworkElement* container);

    _Check_return_ HRESULT ToolTipService_OnOwnerPointerEntered(_In_ CDependencyObject* sender, _In_ CPointerEventArgs* args);

    _Check_return_ HRESULT ToolTipService_OnOwnerPointerExitedOrLostOrCanceled(_In_ CDependencyObject* sender, _In_ CPointerEventArgs* args);

    _Check_return_ HRESULT ToolTipService_OnOwnerGotFocus(_In_ CDependencyObject* sender, _In_ CRoutedEventArgs* args);

    _Check_return_ HRESULT ToolTipService_OnOwnerLostFocus(_In_ CDependencyObject* sender, _In_ CRoutedEventArgs* args);

    _Check_return_ HRESULT XamlCompositionBrushBase_OnConnected(_In_ CDependencyObject* object);

    _Check_return_ HRESULT XamlCompositionBrushBase_OnDisconnected(_In_ CDependencyObject* object);

    _Check_return_ HRESULT AutoSuggestBox_OnInkingStarted(_In_ CDependencyObject* autoSuggestBox, _In_ CDependencyObject* inkingBox);

    _Check_return_ HRESULT AutoSuggestBox_OnInkingEnded(_In_ CDependencyObject* autoSuggestBox, _In_ CDependencyObject* inkingBox);

    _Check_return_ HRESULT AutoSuggestBox_OnInkingFunctionButtonClicked(_In_ CDependencyObject* autoSuggestBox);

    _Check_return_ HRESULT XamlCompositionBrushBase_OnElementConnected(_In_ CDependencyObject* object, _In_ CDependencyObject* connectedElement);

    bool XamlCompositionBrushBase_HasPrivateOverrides(_In_ CDependencyObject* object);

    _Check_return_ HRESULT PasswordBox_OnPasswordChangingHandler(_In_ CPasswordBox* const nativePasswordBox, _In_ bool passwordChanged);

    _Check_return_ HRESULT UIElement_RaiseProcessKeyboardAccelerators(
        _In_ CUIElement* pUIElement,
        _In_ wsy::VirtualKey key,
        _In_ wsy::VirtualKeyModifiers keyModifiers,
        _Out_ BOOLEAN *pHandled,
        _Out_ BOOLEAN *pHandledShouldNotImpedeTextInput);

    _Check_return_ HRESULT KeyboardAccelerator_RaiseKeyboardAcceleratorInvoked(
        _In_ CKeyboardAccelerator* pNativeAccelerator,
        _In_ CDependencyObject* pElement,
        _Out_ BOOLEAN *pIsHandled);

    _Check_return_ HRESULT KeyboardAccelerator_SetToolTip(
        _In_ CKeyboardAccelerator* pNativeAccelerator,
        _In_ CDependencyObject* pParentControl);

    _Check_return_ HRESULT TextBox_OnBeforeTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ xstring_ptr* newString, _Out_ BOOLEAN* wasCanceled);

    _Check_return_ HRESULT TextBox_OnSelectionChangingHandler(
        _In_ CDependencyObject* const nativeTextBox,
        _In_ long selectionStart,
        _In_ long selectionLength,
        _Out_ BOOLEAN* wasCanceled);

    _Check_return_ HRESULT RichEditBox_OnSelectionChangingHandler(
        _In_ CDependencyObject* const nativeRichEditBox,
        _In_ long selectionStart,
        _In_ long selectionLength,
        _Out_ BOOLEAN* wasCanceled);

    bool CompositionTarget_HasHandlers();

    _Check_return_ HRESULT UIElement_OnBringIntoViewRequested(_In_ CUIElement* pUIElement, _In_ CRoutedEventArgs* args);
    void JupiterWindow_SetPointerCapture();
    bool JupiterWindow_HasPointerCapture();
    void JupiterWindow_ReleasePointerCapture();
    bool JupiterWindow_IsWindowDestroyed();
    void JupiterWindow_SetFocus();

    DirectUI::FocusVisualKind FrameworkApplication_GetFocusVisualKind();

    Theming::Theme FrameworkApplication_GetApplicationRequestedTheme();

    void PopCaretBrowsingDialog(_In_ IInspectable* xamlRoot);

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    _Check_return_ HRESULT InteractionCollection_HasInteractionForEvent(KnownEventIndex eventId, _In_ CUIElement* sender, _Out_ bool& hasInteraction);
    _Check_return_ HRESULT InteractionCollection_DispatchInteraction(KnownEventIndex eventId, _In_ CUIElement* sender, _In_ CEventArgs* args);
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

    // CONTENT-TODO: OneCoreTransforms is not supported by lifted IXP at this time.
    _Check_return_ HRESULT DxamlCore_OnCompositionContentStateChangedForUWP();

    _Check_return_ HRESULT DXamlCore_SetBinding(_In_ CDependencyObject* source, _In_ HSTRING path, _In_ CDependencyObject* target, KnownPropertyIndex targetPropertyIndex);

    _Check_return_ HRESULT DXamlCore_GetVisibleContentBoundsForElement(_In_opt_ CDependencyObject* element, _Out_ wf::Rect* value);

    _Check_return_ HRESULT DXamlCore_GetContentBoundsForElement(_In_opt_ CDependencyObject* element, _Out_ wf::Rect* value);
    _Check_return_ HRESULT DXamlCore_CalculateAvailableMonitorRect(_In_ CUIElement* pTargetElement,
        _In_ wf::Point targetPointClientLogical,
        _Out_ wf::Rect* availableMonitorRectClientLogicalResult);

    _Check_return_ HRESULT DXamlCore_GetUISettings(_Out_ ctl::ComPtr<wuv::IUISettings>& spUISettings);

    void XamlIslandRoot_OnSizeChanged(_In_ CXamlIslandRoot* xamlIslandRoot);
    CXamlIslandRoot* GetXamlIslandRootFromXamlIsland(_In_ xaml_hosting::IXamlIslandRoot* xamlIslandRoot);

    _Check_return_ HRESULT FrameworkApplication_RemoveIsland(_In_ xaml_hosting::IXamlIslandRoot* pIsland);

    _Check_return_ HRESULT TextControlFlyout_ShowAt(_In_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* target, wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode flyoutShowMode);

    _Check_return_ HRESULT TextControlFlyout_AddProofingFlyout(_In_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* target);

    _Check_return_ HRESULT TextControlFlyout_CloseIfOpen(_In_opt_ CFlyoutBase* flyoutBase);

    bool TextControlFlyout_IsGettingFocus(_In_opt_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* owner);

    bool TextControlFlyout_IsElementChildOfOpenedFlyout(_In_opt_ CUIElement* element);

    bool TextControlFlyout_IsElementChildOfTransientOpenedFlyout(_In_opt_ CUIElement* element);

    bool TextControlFlyout_IsElementChildOfProofingFlyout(_In_opt_ CUIElement* element);

    _Check_return_ HRESULT TextControlFlyout_DismissAllFlyoutsForOwner(_In_opt_ CUIElement* element);

    bool TextControlFlyout_IsOpen(_In_ CFlyoutBase* flyoutBase);

    _Check_return_ HRESULT TextBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl);

    _Check_return_ HRESULT TextBlock_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl);

    _Check_return_ HRESULT RichEditBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl);

    _Check_return_ HRESULT RichTextBlock_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl);

    _Check_return_ HRESULT PasswordBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl);

    IInspectable* XamlRoot_Create(_In_ VisualTree* visualTree);
    void XamlRoot_RaiseChanged(_In_ IInspectable* xamlRootInsp);
    void XamlRoot_RaiseInputActivationChanged(_In_ IInspectable* xamlRootInsp);
    void XamlRoot_UpdatePeg(_In_ IInspectable* xamlRootInsp, bool peg);

    _Check_return_ HRESULT FrameworkApplication_GetRequiresPointerMode(_Out_ xaml::ApplicationRequiresPointerMode* value);

    _Check_return_ HRESULT FlyoutPresenter_GetTargetIfOpenedAsTransient(_In_ CDependencyObject* nativeControl, _Outptr_ CDependencyObject** nativeTarget);

    _Check_return_ HRESULT FrameworkApplication_GetResourceManagerOverrideFromApp(_Out_ wrl::ComPtr<mwar::IResourceManager>& resourceManager);
}